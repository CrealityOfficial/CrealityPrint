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
#include "print_manage/data/DataType.hpp"

#define FILAMENT_BTN_WIDTH  110
#define FILAMENT_BTN_HEIGHT 41


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

	void update_sync_box_state(bool sync, const wxString& box_filament_name = "");
	void update_child_button_color(const wxColour& color);
    void resetCFS(bool bCFS);
	void update_child_button_size();

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
	void on_left_down(wxMouseEvent &evt);

public:

	Slic3r::GUI::PlaterPresetComboBox* m_filamentCombox;
    ScalableButton* m_img_extruderTemp;
    ScalableButton* m_img_bedTemp;
    Label*                             m_lb_extruderTemp = nullptr;
    Label*                             m_lb_bedTemp      = nullptr;

    ScalableButton* m_edit_btn;
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
	void update_button_size();
	
    wxString    name();
    wxString    boxname();
    wxColour    color();

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
	void on_auto_mapping_filament(const DM::Device& deviceData);
	void update_box_filament_sync_state(bool sync);
	void reset_filament_sync_state();
    std::string get_filament_map_string();
    void resetFilamentToCFS();
    void updateLastFilament(const std::vector<std::string>& presetName);
	void on_sync_one_filament(int filament_index, const std::string& new_filament_color, const std::string& new_filament_name, const wxString& sync_label);
	void backup_extruder_colors();
	void restore_prev_extruder_colors();

    std::vector<FilamentItem*> get_filament_items();

private:
    json m_FilamentProfileJson;
    int LoadFilamentProfile(bool isCxVedor=true);
    void SetFilamentProfile(std::vector<std::pair<int, DM::Material>>& validMaterials);

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
        if (!popup) return;

        m_popups.push_back(popup);

        // 绑定事件
        popup->Bind(wxEVT_DESTROY, &PopupWindowManager::OnPopupDestroyed, this);
    }

    void CloseLast()
    {
        // 创建副本避免迭代器失效
        std::vector<PopupWindow*> popupsCopy = m_popups;
        //m_popups.clear();  // 立即清空原列表，防止重复处理
        m_popups.erase(m_popups.end() - 1);
        PopupWindow* popup = popupsCopy.back();
        //for (PopupWindow* popup : popupsCopy) {
            if (popup) {
                // 确保先解除事件绑定
                popup->Unbind(wxEVT_DESTROY, &PopupWindowManager::OnPopupDestroyed, this);

                // 关闭并销毁弹窗
                popup->Dismiss();
                popup->Destroy();
            }
        //}
    }
    void CloseAll()
    {
        // 创建副本避免迭代器失效
        std::vector<PopupWindow*> popupsCopy = m_popups;
        m_popups.clear();  // 立即清空原列表，防止重复处理

        for (PopupWindow* popup : popupsCopy) {
            if (popup) {
                // 确保先解除事件绑定
                popup->Unbind(wxEVT_DESTROY, &PopupWindowManager::OnPopupDestroyed, this);

                // 关闭并销毁弹窗
                popup->Dismiss();
                popup->Destroy();
            }
        }
    }
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
    ManagedPopupWindow(wxWindow* parent) : PopupWindow(parent, wxBORDER_NONE) { 
        SetBackgroundStyle(wxBG_STYLE_PAINT); // 启用自定义绘制
        SetDoubleBuffered(true); // 启用双缓冲防止闪烁
        Bind(wxEVT_PAINT, &ManagedPopupWindow::OnPaint, this);
        init();
    }
    void init();
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
    //void Dismiss() override
    //{
    //    PopupWindowManager::Get().CloseAll();
    //    PopupWindow::Dismiss();
    //}
    void OnPaint(wxPaintEvent& event);
};


class MaterialSubMenuItem : public wxWindow
{
public:
    MaterialSubMenuItem(wxWindow* parent, const wxString& label, const wxColour& color, const int num);
    ~MaterialSubMenuItem() = default;
	void setParentIndex(int index) { m_parentindex = index; }
private:
	int      m_parentindex = -1; // 父菜单索引
    int      m_num = 0;
    wxString m_label;
    wxColour m_color;
    bool     m_hovered = false;
    bool     m_clicked = false;

    void OnPaint(wxPaintEvent&);

    void OnMouseRelease(wxMouseEvent&);

    void OnMousePressed(wxMouseEvent&);
    void OnMouseEnter(wxMouseEvent&);

    void OnMouseLeave(wxMouseEvent&);
};

// 自定义按钮类实现状态管理
class HoverButton : public wxButton
{
public:
    HoverButton(wxWindow* parent,
        wxWindowID      id,
        const wxString& label,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        const int& type = 0);

    void SetBaseColors(const wxColour& normal, const wxColour& pressed);

    void SetBitMap_Cus(wxBitmap bit1, wxBitmap bit2);
    void SetExpendStates(bool expend);
private:
    wxColour m_baseColor;
    wxColour m_pressedColor;
    wxBitmap bitmap = wxNullBitmap;
    wxBitmap bitmap_hover = wxNullBitmap;
    wxSize m_size = wxDefaultSize;
    bool isHover = false;
    bool    m_isExpend = false;

    void BindEvents();
    void OnLeftDown(wxMouseEvent& e);

    void OnLeftUp(wxMouseEvent& e);
    void OnEnter(wxMouseEvent& e);
    
    void OnLeave(wxMouseEvent& e);
    
   
    void OnPaint(wxPaintEvent&);
    int m_type = 0;    //0:del,1:merge
};

// 子菜单窗口
class MaterialSubMenu : public ManagedPopupWindow
{
public:
    MaterialSubMenu(wxWindow* parent,int index = -1);

    void init();

private:
    int m_index = 0;
    ManagedPopupWindow* m_menuPop = nullptr;
    void Dismiss() override {
        wxPoint mousePos = ::wxGetMousePosition();
        // 判断点击位置是否在弹窗外
        if (m_menuPop && !m_menuPop->GetScreenRect().Contains(mousePos)) {
            PopupWindowManager::Get().CloseAll();
        }
    }
};
// 自定义右键菜单窗口
class MaterialContextMenu : public ManagedPopupWindow
{
public:
    MaterialContextMenu(wxWindow* parent,int index);

private:
    HoverButton* m_mergeBtn;
    int       m_index = 0;
	bool        m_is_clicked = false;
    void            OnShowSubmenu(wxCommandEvent&);
    void            OnDelete(wxCommandEvent&);
    bool    m_isExpended = false;
};

#endif // 
