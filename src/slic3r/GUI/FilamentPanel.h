#ifndef FILAMENTPANEL_H
#define FILAMENTPANEL_H

#include <string>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/window.h>
#include "wx/string.h"
#include "wx/wrapsizer.h"
#include "Widgets/PopupWindow.hpp" 
#include "libslic3r/PresetBundle.hpp"
#include "PresetComboBoxes.hpp"
#include "Widgets/Label.hpp"
#include "slic3r/GUI/print_manage/DeviceDB.hpp"

namespace Slic3r { 

namespace GUI {
	struct BoxColorSelectPopupData {
		wxPoint popup_position;
		int filament_item_index;
	};
}

}

/*
* FilamentButtonStateHandler
*/
wxDECLARE_EVENT(EVT_ENABLE_CHANGED, wxCommandEvent);
class FilamentButtonStateHandler : public wxEvtHandler
{
public:
	enum State {
		Normal = 0,
		Hover = 1,
	};

public:
	FilamentButtonStateHandler(wxWindow* owner);
	~FilamentButtonStateHandler();
public:
	void update_binds();
	int states() const { return m_states; }


private:
	FilamentButtonStateHandler(FilamentButtonStateHandler* parent, wxWindow* owner);
	void changed(wxEvent& event);

private:
	wxWindow* owner_;
	int bind_states_ = 0;
	int m_states = 0;
};


/*
* FilamentButton
*/
class FilamentButton : public wxWindow
{
public:
	FilamentButton(wxWindow* parent,
		wxString text,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = 0);

	void SetCornerRadius(double radius);
	void SetBorderWidth(int width);
	void SetColor(wxColour bk_color);
    void SetIcon(wxString dark_icon, wxString light_icon);
    void SetLabel(wxString lb);
	void update_sync_box_state(bool sync, const wxString& box_filament_name = "");
	void update_child_button_color(const wxColour& color);
    void resetCFS(bool bCFS);

protected:

	void mouseDown(wxMouseEvent& event);
	void mouseReleased(wxMouseEvent& event);
	void eraseEvent(wxEraseEvent& evt);
	void paintEvent(wxPaintEvent& evt);
	void render(wxDC& dc);
	virtual void doRender(wxDC& dc);

	void OnChildButtonClick(wxMouseEvent& event);
    void OnChildButtonPaint(wxPaintEvent& event);

protected:
	double m_radius;
	int m_border_width = 1;
	FilamentButtonStateHandler m_state_handler;
	wxColour m_back_color;
    bool                       m_bReseted = true;
    wxColour                   m_resetedColour = wxColour("#FFFFFF");
	wxString m_label;
    ScalableBitmap m_dark_img;
    ScalableBitmap m_light_img;
	wxString m_sync_filament_label = "cfs";
	bool m_sync_box_filament = false;

	wxButton* m_child_button {nullptr};
	wxBitmap m_bitmap;

	DECLARE_EVENT_TABLE()
};

/*
* FilamentPopPanel
*/
class FilamentItem;
class FilamentPopPanel : public PopupWindow
{
public:
	FilamentPopPanel(wxWindow* parent, int index);
	~FilamentPopPanel();

	void Popup(wxPoint position = wxDefaultPosition);
	void Dismiss();
    void sys_color_changed();
    void setFilamentItem(FilamentItem* pFilamentItem) { m_pFilamentItem = pFilamentItem; }

public:

	Slic3r::GUI::PlaterPresetComboBox* m_filamentCombox;
    ScalableButton* m_img_extruderTemp;
    ScalableButton* m_img_bedTemp;
    Label* m_lb_extruderTemp;
    Label* m_lb_bedTemp;

    ScalableButton* m_edit_btn;
	wxColour m_bg_color;
	wxBoxSizer* m_sizer_main{ nullptr };
    int	m_index=-1;
    FilamentItem*   m_pFilamentItem = nullptr;
};

/*
* FilamentItem
*/
class FilamentItem : public wxPanel
{
public:
    struct Data
    {
        int index = 0;
        std::string name; 
        bool small_state = false;
    };
public:
    FilamentItem(wxWindow* parent, const Data&data, const wxSize&size=wxSize(110, 41));

    void set_checked(bool checked = true);
    bool is_checked();

    bool to_small(bool bSmall = true);
	void update();
    void sys_color_changed();
    void msw_rescale();
	void paintEvent(wxPaintEvent& evt);
	int index();
	void update_bk_color(const std::string& bk_color);
	void set_filament_selection(const wxString& filament_name);
	void update_box_sync_state(bool sync, const wxString& box_filament_name = "");
	void update_box_sync_color(const std::string& sync_color);
    void resetCFS(bool bCFS);
	
private:
    wxBoxSizer* m_sizer;
    FilamentButton* m_btn_color;
    FilamentButton* m_btn_param_list;

    wxColour m_bk_color;
    wxColour m_checked_border_color;
    bool m_checked_state{false};

    int m_radius = 3;
    int m_border_width = 1;

    FilamentPopPanel* m_popPanel;
    bool m_small_state = false;
	bool m_sync_box_filament = false;

	FilamentItem::Data m_data;

	Slic3r::PresetBundle* m_preset_bundle{nullptr};
    Slic3r::PresetCollection* m_collection{nullptr};
    DECLARE_EVENT_TABLE()
};

/*
* FilamentPanel
*/
class FilamentPanel : public wxPanel
{
public:
	FilamentPanel(wxWindow* parent,
		wxWindowID      id = wxID_ANY,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = 0);

	bool add_filament();
    bool can_add();
	bool can_delete();
	void del_filament(int index = -1);
	void to_small(bool bSmall = true);
    void update(int index=-1);
    void sys_color_changed();
    void msw_rescale();
    size_t size();
	void on_re_sync_all_filaments(const std::string& selected_device_ip);
	void on_auto_mapping_filament(const RemotePrint::DeviceDB::Data& deviceData);
	void update_box_filament_sync_state(bool sync);
	void reset_filament_sync_state();
	void on_sync_one_filament(int filament_index, const std::string& new_filament_color, const std::string& new_filament_name, const wxString& sync_label);
	void backup_extruder_colors();
	void restore_prev_extruder_colors();

private:
    json m_FilamentProfileJson;
    int LoadFilamentProfile();
    bool LoadFile(std::string jPath, std::string& sContent);
    int LoadProfileFamily(std::string strVendor, std::string strFilePath);
    int GetFilamentInfo(std::string VendorDirectory, json& pFilaList, std::string filepath, std::string& sVendor, std::string& sType);
    void SetFilamentProfile(std::vector<std::pair<int, RemotePrint::DeviceDB::Material>>& validMaterials);

protected:
	void paintEvent(wxPaintEvent& evt);
	
private:
	wxWrapSizer* m_sizer;
	wxBoxSizer*m_box_sizer;
	int m_max_count = { 64 };
    int m_small_count = { 64 };
	std::vector<FilamentItem*> m_vt_filament;

	// when current device changed(from multiColor box to singleColor box), restore filament color
    std::vector<std::string> m_backup_extruder_colors;
};

// draw one color rectangle and text "1A" or "1B" or "1C" or "1D"
class FilamentColorSelectionItem : public wxButton
{

public:
    FilamentColorSelectionItem(wxWindow* parent, const wxSize& size);
    ~FilamentColorSelectionItem();

    void SetColor(const wxColour& color);
    wxColour GetColor();

    void update_item_info_by_material(int box_id, const RemotePrint::DeviceDB::Material& material_info);
    void set_sync_state(bool bSync);
	bool get_sync_state();
    void set_is_ext(bool is_ext);
	wxString get_filament_type_label();
	wxString get_material_index_info();
	std::string get_filament_name();

protected:
    void OnPaint(wxPaintEvent& event);

private:
    wxColour m_bk_color;
    int m_box_id;
    int m_material_id;
    wxString m_material_index_info;  // 1A, 1B, 1C, 1D
	wxString m_filament_type_label;  // "PLA" or "ABS" or "PETG" ...
	std::string m_filament_name;
    int m_radius = 8;
    int m_border_width = 1;
    bool m_sync = false;
    bool m_is_ext = false;
};


/*
* BoxColorPopPanel
*/
class BoxColorPopPanel : public PopupWindow
{
public:
	BoxColorPopPanel(wxWindow* parent);
	~BoxColorPopPanel();

	void set_filament_item_index(int index);
	void init_by_device_data(const RemotePrint::DeviceDB::Data& device_data);
	void select_first_on_show();

protected:
    void OnMouseEnter(wxMouseEvent& event);
    void OnMouseLeave(wxMouseEvent& event);
	void OnFirstColumnButtonClicked(wxCommandEvent& event);
	void OnSecondColumnItemClicked(wxCommandEvent& event);

private:
    wxBoxSizer* m_mainSizer;
    wxBoxSizer* m_firstColumnSizer;
    wxBoxSizer* m_secondColumnSizer;
    wxPanel* m_secondColumnPanel;

	int m_filament_item_index = 0;
	RemotePrint::DeviceDB::Data m_device_data;

    wxDECLARE_EVENT_TABLE();

};

#endif // 
