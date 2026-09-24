#ifndef FILAMENTPANEL_H
#define FILAMENTPANEL_H

#include <string>
#include <wx/panel.h>
#include <wx/scrolwin.h>
#include <wx/sizer.h>
#include <wx/window.h>
#include <wx/menu.h>
#include "wx/string.h"
#include "wx/wrapsizer.h"
#include "Widgets/PopupWindow.hpp" 
#include "libslic3r/PresetBundle.hpp"
#include "PresetComboBoxes.hpp"
#include "Widgets/Label.hpp"
#include "print_manage/data/DataType.hpp"
#include <functional>
#include <map>
#include <memory>

#define FILAMENT_BTN_WIDTH  110
#define FILAMENT_BTN_HEIGHT 46

class Button;

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
    wxString getLabel();
	// Keeps the label in the upper row's left slot instead of centring it across
	// the whole colour block.
	void SetLabelTopLeft(bool top_left);

	void update_sync_box_state(bool sync, const wxString& box_filament_name = "");
	void update_child_button_color(const wxColour& color);
    void resetCFS(bool bCFS);
	void update_child_button_size();
	// Turns the right-most quarter of the block into a "three dots" overflow menu.
	void enable_menu_button(bool enable);

protected:

	void mouseDown(wxMouseEvent& event);
	void mouseReleased(wxMouseEvent& event);
	void eraseEvent(wxEraseEvent& evt);
	void paintEvent(wxPaintEvent& evt);
	void render(wxDC& dc);
	virtual void doRender(wxDC& dc);

	void OnChildButtonClick(wxMouseEvent& event);
    void OnChildButtonPaint(wxPaintEvent& event);
	void OnMouseMove(wxMouseEvent& event);
	void OnMouseLeave(wxMouseEvent& event);

	void OnSize(wxSizeEvent& event);
	void layout_child_windows();
	void set_top_hover_region(int region);
	// Left slot occupied by the filament number.
	wxRect label_area_rect() const;
	// Right-most quarter of the block, owned by the overflow menu (hit area).
	wxRect menu_area_rect() const;
	// Square chip drawn inside menu_area_rect().
	wxRect menu_plate_rect() const;
	// Shared height of the inner plates (CFS / "...").
	int inner_plate_height() const;
	void draw_menu_area(wxDC& dc);
	void draw_top_hover_area(wxDC& dc);
	void show_menu();
	// Checkerboard fill used to visualize a fully transparent filament colour.
	wxBrush make_transparency_brush(int tile_dip) const;
	// Background colour currently painted behind the owner-drawn children.
	const wxColour& child_background_colour() const;
    
protected:
	double m_radius;
	int m_border_width = 1;
	FilamentButtonStateHandler m_state_handler;
	wxColour m_back_color;
	wxString m_label;
	bool m_label_top_left = false;
    ScalableBitmap m_dark_img;
    ScalableBitmap m_light_img;
	wxString m_sync_filament_label = "cfs";
	bool m_sync_box_filament = false;
	// Hover target in the upper row: 0 none, 1 index, 2 CFS, 3 more.
	int m_top_hover_region = 0;

	// Linux/GTK 下在 wxButton 上做自绘不可靠，改为 wxPanel 做 owner-draw
	wxPanel* m_child_button {nullptr};
	wxBitmap m_bitmap;

	// "..." overflow menu, drawn directly on this window (no child window, so the
	// block's rounded border stays intact). Only enabled for the color block.
	bool m_menu_area_enabled {false};

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

	void Dismiss();
    void msw_rescale(wxWindow* dpi_reference = nullptr);
    void sys_color_changed();
    void setFilamentItem(FilamentItem* pFilamentItem) { m_pFilamentItem = pFilamentItem; }
	// Opens the preset list of the hosted combobox directly, without ever
	// showing this panel.
	void PopupPresetList();

public:

	Slic3r::GUI::PlaterPresetComboBox* m_filamentCombox;
	wxColour m_bg_color;
	wxBoxSizer* m_sizer_main{ nullptr };
    int	m_index=-1;
    FilamentItem*   m_pFilamentItem = nullptr;
};

const wxColour MENU_COLORS[8] = {
    wxColour(255, 0, 0),     // 01
    wxColour(144, 238, 144), // 02
    wxColour(0, 255, 0),     // 03
    wxColour(255, 0, 255),   // 04
    wxColour(255, 0, 0),     // 05
    wxColour(0, 0, 255),     // 06
    wxColour(173, 216, 230), // 07
    wxColour(128, 128, 128)  // 08
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
        std::string box_filament_name;
        bool small_state = false;
    };
public:
    FilamentItem(wxWindow* parent, const Data&data, const wxSize&size=wxSize(FILAMENT_BTN_WIDTH, FILAMENT_BTN_HEIGHT));
    ~FilamentItem() override;

    void set_checked(bool checked = true);
    bool is_checked();

    bool to_small(bool bSmall = true);
	void update(bool persist_changes = true);
    void sys_color_changed();
    void msw_rescale();
	void paintEvent(wxPaintEvent& evt);
	int index();
	void update_bk_color(const std::string& bk_color);
	std::string set_filament_selection(const wxString& filament_name, bool notify = true);
	void update_box_sync_state(bool sync, const wxString& box_filament_name = "");
	void update_box_sync_color(const std::string& sync_color);
    void resetCFS(bool bCFS);
	void update_button_size();
	// Opens the filament settings page for this slot (same as the pencil button
	// inside the expanded parameter popup).
	void edit_preset();
	void set_nozzle_no(int nozzle_no) { m_nozzle_no = nozzle_no < 1 ? 1 : nozzle_no; }
	int nozzle_no() const { return m_nozzle_no; }
	
    wxString    name();
    wxString    boxname();
    wxColour    color();
    wxString    preset_name();

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
    wxString m_preset_name;
    int m_nozzle_no { 1 };

    DECLARE_EVENT_TABLE()
};



/*
* FilamentPanel
*/
class FilamentPanel : public wxPanel
{
public:
	using AutoMappingCompletion = std::function<void(bool, const std::string&)>;
	using AutoMappingValidator  = std::function<bool()>;

	FilamentPanel(wxWindow* parent,
		wxWindowID      id = wxID_ANY,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = 0);
	~FilamentPanel() override;

	bool add_filament();
    bool can_add();
	bool can_delete();
    void clear_all();
	void del_filament(int index = -1);
	void to_small(bool bSmall = true);
    void update(int index=-1);
    bool prepare_filament_nozzle_mapping_for_slice(bool will_post_slice_event = false, bool slice_all = false, int plate_index = -1);
    void open_filament_grouping_dialog();
    void reflow_for_width();
    void sys_color_changed();
    void msw_rescale();
    size_t size();
	void on_re_sync_all_filaments(const std::string& selected_device_ip);
	void on_auto_mapping_filament(const DM::Device& deviceData,
	                              AutoMappingCompletion completion = {},
	                              AutoMappingValidator validator = {});
	void update_box_filament_sync_state(bool sync);
	void reset_filament_sync_state();
    void reset_device_filament_mapping_to_cfs();
    std::string get_filament_map_string();
    void resetFilamentToCFS();
    void updateLastFilament(const std::vector<std::string>& presetName);
	void on_sync_one_filament(int filament_index, const std::string& new_filament_color, const std::string& new_filament_name, const wxString& sync_label);
    void sync_box_filament_state(int filament_index, const std::string& new_filament_color, const wxString& sync_label);
	void backup_extruder_colors();
	void restore_prev_extruder_colors();

    std::vector<FilamentItem*> get_filament_items();

private:
    void on_filament_wheel(wxMouseEvent& event);
    void update_scroll_height();
    bool supports_filament_nozzle_mapping() const;
    size_t nozzle_count_for_mapping() const;
    void refresh_filament_grouping_visibility();
    std::vector<size_t> used_filament_ids_for_grouping(bool slice_all, int plate_index = -1) const;
    Slic3r::FilamentMapAutoInput make_filament_map_auto_input(bool slice_all, int plate_index = -1) const;
    bool apply_current_filament_nozzle_mapping(bool slice_all, int plate_index = -1);
    bool show_filament_grouping_dialog(bool slice_all, bool* mapping_changed = nullptr, int plate_index = -1);

    bool SetFilamentProfile(const std::map<std::string, std::string>& section_new);
    void apply_auto_mapping_filament(std::vector<std::pair<int, DM::Material>> validMaterials,
                                     bool is_cfs_mini,
                                     std::map<std::string, std::string> section_new,
                                     bool profile_available);

protected:
	void paintEvent(wxPaintEvent& evt);
	
private:
	wxWrapSizer* m_sizer;
	wxBoxSizer*m_box_sizer;
    wxScrolledWindow* m_filament_scrolled { nullptr };
    wxPanel* m_filament_content { nullptr };
    int m_filament_wheel_rotation { 0 };
	Button* m_grouping_btn { nullptr };
	// Lets the event posted after pre-slice confirmation continue without reopening the dialog.
	bool m_skip_next_filament_nozzle_mapping_dialog { false };
    bool m_filament_nozzle_mapping_in_progress { false };
	int m_max_count = { 64 };
    int m_small_count = { 64 };
	std::vector<FilamentItem*> m_vt_filament;
	std::shared_ptr<int> m_lifetime_token = std::make_shared<int>(0);

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

    void update_item_info_by_material(int box_id, const DM::Material& material_info,int box_type = 0);  //0=多色盒子 1=外置料架 2=cfsMini
    void set_sync_state(bool bSync);
	bool get_sync_state();
    void set_is_ext(bool is_ext);
	wxString get_filament_type_label();
	wxString get_material_index_info();
	std::string get_filament_name();
    std::string getUserMaterial();

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
    std::string m_userMaterial;
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
	void init_by_device_data(const DM::Device& device_data);
	void select_first_on_show();
	void on_left_down(wxMouseEvent &evt);

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
	DM::Device m_device_data;

    wxDECLARE_EVENT_TABLE();

};

// 弹出窗口管理器（单例）
class PopupWindowManager
{
public:
    static PopupWindowManager& Get()
    {
        static PopupWindowManager instance;
        return instance;
    }

    void RegisterPopup(PopupWindow* popup)
    {
        if (!popup || std::find(m_popups.begin(), m_popups.end(), popup) != m_popups.end()) return;

        m_popups.push_back(popup);

        // 绑定事件
        popup->Bind(wxEVT_DESTROY, &PopupWindowManager::OnPopupDestroyed, this);
    }

    void CloseLast()
    {
        if (m_popups.empty())
            return;
        PopupWindow* popup = m_popups.back();
        m_popups.pop_back();
        popup->Unbind(wxEVT_DESTROY, &PopupWindowManager::OnPopupDestroyed, this);
        // Explicit close must not re-enter the activation-based Dismiss override.
        popup->PopupWindow::Dismiss();
        popup->Destroy();
    }

    void CloseAll()
    {
        // Destroy children before their owning popup.
        while (!m_popups.empty())
            CloseLast();
    }
#ifdef __WXMSW__
    bool IsMenuActive() const;
#endif
private:
    std::vector<PopupWindow*> m_popups;
    // 弹窗销毁事件处理
    void OnPopupDestroyed(wxWindowDestroyEvent& event) {
        PopupWindow* popup = static_cast<PopupWindow*>(event.GetEventObject());
        auto it = std::find(m_popups.begin(), m_popups.end(), popup);
        if (it != m_popups.end()) {
            m_popups.erase(it);
        }
        event.Skip();  // 允许其他处理
    }
};

// 增强型弹出窗口基类
class ManagedPopupWindow : public PopupWindow
{
public:
    ManagedPopupWindow(wxWindow* parent, int style = wxBORDER_NONE | wxPU_CONTAINS_CONTROLS) : PopupWindow(parent, style) {
        SetBackgroundStyle(wxBG_STYLE_PAINT); // 启用自定义绘制
        SetDoubleBuffered(true); // 启用双缓冲防止闪烁
        Bind(wxEVT_PAINT, &ManagedPopupWindow::OnPaint, this);
        init();
    }
    void init();
#ifdef __WXMSW__
    void Dismiss() override;
#endif
    void Popup(wxWindow* focus = NULL) override
    {
        // 先关闭所有已有弹窗
        PopupWindowManager::Get().CloseAll();

        // 注册新弹窗
        PopupWindowManager::Get().RegisterPopup(this);

        PopupWindow::Popup(focus);
    }
    //直接自定义一个
    void Cus_Popup(bool needshow_parent = false, wxWindow* focus = NULL)
    {
        // 先关闭所有已有弹窗
        if (!needshow_parent)
            PopupWindowManager::Get().CloseAll();

        // 注册新弹窗
        PopupWindowManager::Get().RegisterPopup(this);
#ifdef __APPLE__
    PopupWindow::Show();
#else
    PopupWindow::Popup(focus);
#endif
    }

protected:
    void OnPaint(wxPaintEvent& event);
};


// Native menu; actions run after GetPopupMenuSelectionFromUser() closes the menu.
class MaterialContextMenu : public wxMenu
{
public:
    MaterialContextMenu(wxWindow* parent, int index);
    void ExecuteSelection(int id);

private:
    int m_index = 0;
    std::map<int, std::function<void()>> m_actions;
    void OnEdit();
    void OnDelete();
    void OnDecomposeColor();
};

#endif //
