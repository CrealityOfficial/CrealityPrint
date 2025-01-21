#ifndef slic3r_params_panel_hpp_
#define slic3r_params_panel_hpp_


#include <cstddef>
#include <map>
#include <vector>
#include <memory>


#include <unordered_set>
#include <wx/artprov.h>
#include <wx/statbox.h>
#include <wx/xrc/xmlres.h>
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/tglbtn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/timer.h>
#include <wx/wupdlock.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/scrolwin.h>
#include <wx/panel.h>

#include "libslic3r/Preset.hpp"
#include "wxExtensions.hpp"
#include "GUI_Utils.hpp"
#include "Widgets/Button.hpp"

class SwitchButton;
class StaticBox;
class HoverBorderBox;
class HoverBorderIcon;

#define TIPS_DIALOG_BUTTON_SIZE wxSize(FromDIP(60), FromDIP(24))

class TabCtrl;
namespace Slic3r {

class Preset;

namespace GUI {

class HTabbarPrinter;
class VTabbarPrinter;
class TabbarCtrlPrinter;
class TabbarCtrlFilament;
class TabbarController;

///////////////////////////////////////////////////////////////////////////

class ImageTooltipPanel : public wxPanel
{
public:
    ImageTooltipPanel(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style = wxTAB_TRAVERSAL);

    ~ImageTooltipPanel();

    void SetBackgroundImage(const wxString& imagePath, const wxString& textTooltip);
    void on_change_color_mode(bool is_dark);

protected:
    void OnHideButtonClick(wxCommandEvent& event);

    void OnPaint(wxPaintEvent& event);

private:
    wxStaticBitmap* m_backgroundImage{nullptr};
    wxStaticText* m_text{nullptr};
    wxBoxSizer* m_mainSizer{nullptr};
    wxBoxSizer* m_textButtonSizer {nullptr};
    wxBitmap* m_defaultBmp {nullptr};
    wxButton* m_hideButton {nullptr};
    wxString m_imagePath;
};

class TipsDialog : public DPIDialog
{
private:
    bool m_show_again{false};
    std::string m_app_key;

public:
    TipsDialog(wxWindow *parent, const wxString &title, const wxString &description, std::string app_key = "");
    Button *m_confirm{nullptr};
    Button *m_cancel{nullptr};
    wxPanel *m_top_line{nullptr};
    wxStaticText *m_msg;

protected:
    void on_dpi_changed(const wxRect &suggested_rect) override;
    void on_ok(wxMouseEvent &event);
    wxBoxSizer *create_item_checkbox(wxString title, wxWindow *parent, wxString tooltip, std::string param);
};

///////////////////////////////////////////////////////////////////////////////
/// Class ParamsPanel
///////////////////////////////////////////////////////////////////////////////
class ParamsPanel : public wxPanel
{
#if __WXOSX__
    wxWindow*            m_tmp_panel;
    int                 m_size_move = -1;
#endif // __WXOSX__

	private:
        void free_sizers();
        void delete_subwindows();
        void refresh_tabs();

	protected:
        wxBoxSizer* m_top_sizer { nullptr };
        wxBoxSizer* m_left_sizer { nullptr };
        wxBoxSizer* m_mode_sizer { nullptr };
        // // BBS: new layout
        StaticBox* m_top_panel{ nullptr };
        ScalableButton* m_process_icon{ nullptr };
        wxStaticText* m_title_label { nullptr };
        SwitchButton* m_mode_region { nullptr };
        ScalableButton *m_tips_arrow{nullptr};
        bool m_tips_arror_blink{false};
        wxStaticText* m_title_view { nullptr };
        SwitchButton* m_mode_view { nullptr };
        HoverBorderBox* m_mode_view_box { nullptr };
        //wxBitmapButton* m_search_button { nullptr };
        wxStaticLine* m_staticline_print { nullptr };
        //wxBoxSizer* m_print_sizer { nullptr };
        HTabbarPrinter* m_h_tabbar_printer { nullptr };
        VTabbarPrinter* m_v_tabbar_printer { nullptr };
        Button* m_button_save = nullptr;
        //Button* m_button_save_as = nullptr;
        Button* m_button_delete = nullptr;
        Button* m_button_reset = nullptr;
        TabbarCtrlPrinter* m_tabbar_printer{ nullptr };
        TabbarCtrlFilament* m_tabbar_filament{ nullptr };
        TabbarController* m_tabbar_ctrl{ nullptr };
        wxPanel* m_tab_print { nullptr };
        wxPanel* m_tab_print_plate { nullptr };
        wxPanel* m_tab_print_object { nullptr };
        wxStaticLine* m_staticline_print_object { nullptr };
        wxPanel* m_tab_print_part { nullptr };
        wxPanel* m_tab_print_layer { nullptr };
        wxStaticLine* m_staticline_print_part { nullptr };
        wxStaticLine* m_staticline_filament { nullptr };
        //wxBoxSizer* m_filament_sizer { nullptr };
        wxPanel* m_tab_filament { nullptr };
        wxStaticLine* m_staticline_printer { nullptr };
        //wxBoxSizer* m_printer_sizer { nullptr };
        wxPanel* m_tab_printer { nullptr };
        wxBoxSizer* m_tmp_sizer{ nullptr };
        TabCtrl* m_cur_tabCtrl{ nullptr };
        TabCtrl* m_printer_tabCtrl{ nullptr };
        TabCtrl* m_filament_tabCtrl{ nullptr };
        //wxStaticLine* m_staticline_buttons { nullptr };
        // BBS: new layout
        wxBoxSizer* m_button_sizer { nullptr };
        wxWindow* m_export_to_file { nullptr };
        wxWindow* m_import_from_file { nullptr };
        //wxStaticLine* m_staticline_middle{ nullptr };
        //wxBoxSizer* m_right_sizer { nullptr };
        wxScrolledWindow* m_page_view { nullptr };
        wxBoxSizer* m_page_sizer { nullptr };
        StaticBox* m_color_border_box { nullptr };

        HoverBorderIcon*		m_setting_btn { nullptr };
        ScalableButton*		m_search_btn { nullptr };
        HoverBorderIcon*		m_compare_btn { nullptr };

        ScalableButton* m_btn_reset{nullptr};
        ImageTooltipPanel* m_img_tooltip_panel { nullptr };
        ScalableButton* m_btn_show_img_tooltip{nullptr};
        wxBoxSizer* m_img_tooltip_sizer { nullptr };

        wxBitmap m_toggle_on_icon;
        wxBitmap m_toggle_off_icon;

        wxPanel* m_current_tab { nullptr };

        bool m_has_object_config { false };

        enum PageState
        {
             PS_BASE,
             PS_SYSTEM,
             PS_USER
        };

        enum WindowState
        {
            WS_PRINTER,
            WS_FILAMENT
        };

        struct Highlighter
        {
            void set_timer_owner(wxEvtHandler *owner, int timerid = wxID_ANY);
            void init(std::pair<wxWindow *, bool *>, wxWindow *parent = nullptr);
            void blink();
            void invalidate();

        private:
            wxWindow *      m_bitmap{nullptr};
            bool *         m_show_blink_ptr{nullptr};
            int            m_blink_counter{0};
            wxTimer        m_timer;
            wxWindow *      m_parent { nullptr };
        } m_highlighter;

        void OnToggled(wxCommandEvent& event);
        void OnToggledImageTooltip(wxCommandEvent& event);
        void on_active_tab_changed();

	public:
		ParamsPanel( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 1800,1080 ), long style = wxTAB_TRAVERSAL, const wxString& type = wxEmptyString );
		~ParamsPanel();

        void rebuild_panels();
        virtual void create_layout();
        void create_layout_printerAndFilament();
        void create_layout_process();
        //clear the right page
        void clear_page();
        void OnActivate();
        void set_active_tab(wxPanel*tab);
        bool is_active_and_shown_tab(wxPanel*tab);
        void update_mode(const int mode = -1);
        void msw_rescale();
        void switch_to_global();
        void switch_to_object(bool with_tips = false);
        bool get_switch_of_object();

        void notify_object_config_changed();
        void notify_config_changed();
        void switch_to_object_if_has_object_configs();
        void show_image_tooltip(wxString text_tooltip,wxString img_path);
        void on_change_color_mode(bool is_dark);

        StaticBox* get_top_panel() { return m_top_panel; }

        wxPanel* printer_panel() { return m_tab_printer; }
        wxPanel* filament_panel() { return m_tab_filament; }

        wxScrolledWindow* get_paged_view() { return m_page_view;}
        wxPanel*    get_current_tab() { return m_current_tab; }
        void sys_color_changed();
        ImageTooltipPanel*    get_image_tooltip_panel() { return m_img_tooltip_panel; }
        void tab_page_relayout();
        bool current_tab_is_plate_settings();
        void create_image_tooltip_panel();
        void set_border_panel_color(const wxColour& border_color);

        SwitchButton* get_mode_view() { return m_mode_view; }

        void ShowDetialView(bool show);
        void ShowDeleteButton(bool show);

        void OnPanelShowInit();
        void OnPanelShowExit();
        void OnParentDialogOpen();

        void updateItemState();
        void onChangedSysAndUser(PageState ps);
        void onChangedVentor(std::string ventor);
        void onChangedPrinterType(std::string printer);

        void initData();
        void filteredData(wxString choseVentor, wxString chosePrinterType, int changeType = 0);//0 : all , 1 : vendor, 2 : type, 3 : preset
        std::string findInherits(const std::string& name, Preset::Type type);
        std::string getFilamentVendor(const Preset& preset);
        std::string getVendor(const Preset& preset);
        void  getDatas(std::vector<wxString>& systemPrintList, std::vector<wxString>& userPrintList, std::vector<wxString>& projectList,
            std::unordered_set<wxString>&  printList, std::unordered_set<wxString>&  ventorList);
        void updateVentors(const std::unordered_set<wxString>& ventors);
        void updatePrinters(const std::unordered_set<wxString>& printers);
        void updatePresetsList(const std::vector<wxString>& systemList, const std::vector<wxString>& userList, const std::vector<wxString>& projectList);
        void setCurVentor(const std::string& curVentor);
        void setCurType(const std::string& curType);
        std::string getPresetType(Preset reset);
        void SelectRowByString(const wxString& text);
        bool SelectRowRecursive(const wxDataViewItem& item, const wxString& text);
        std::vector<std::string> filterVentor();
        std::vector<std::string> filterPresetType(const std::string& vector);

        void OnSelectName(const wxString& name);

        void OnSelectPrinterWildVendorModel();
        void OnSelectPrinterWildVendor();
        void OnSelectPrinterVendor(const wxString& vendor);
        void OnSelectPrinterWildModel(const wxString& vendor);
        void OnSelectPrinterVendorModel(const wxString& vendor, const wxString& model);
        void OnSelectPrinterName(const wxString& name);

        void layoutPrinterAndFilament();
        void layoutOther();

    private:
        std::multimap<std::string, std::multimap<std::string, Preset>> m_systemDatas;
        std::multimap<std::string, std::multimap<std::string, Preset>> m_userDatas;
        std::deque<Preset>  m_filtered_presets;
        wxString m_curVentor = wxString();
        wxString m_printerType = wxString();
        wxString m_curPreset = wxString();

        wxColor m_normal_color = wxColor(110, 110, 115);
        wxColor m_hover_color = wxColor(23, 204, 95);

        PageState m_ps = PS_SYSTEM;
        WindowState m_ws = WS_PRINTER;
        wxPanel* m_btnsPanel = nullptr;
        wxButton* m_btn_save = nullptr;
        wxButton* m_btn_saveAs = nullptr;
        wxButton* m_btn_delete = nullptr;
        wxButton* m_btn_resets = nullptr;
        wxButton* m_btn_system = nullptr;
        wxButton* m_btn_user = nullptr;
        wxPanel* m_bottomBtnsPanel = nullptr;
        wxDataViewTreeCtrl* m_preset_listBox = nullptr;
        
        std::unordered_set<wxString>  m_print_list;
        std::unordered_set<wxString>  m_ventor_list;
        std::vector<wxString> m_system_print_list;
        std::vector<wxString> m_user_print_list;
        std::vector<wxString> m_project_print_list;
};

class ProcessParamsPanel : public ParamsPanel
{
public:
    ProcessParamsPanel(wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, 
        const wxSize& size = wxSize(1800, 1080), long style = wxTAB_TRAVERSAL, const wxString& type = wxEmptyString);
    ~ProcessParamsPanel();

    void create_layout() override;
private:

};
} // GUI
} // Slic3r

#endif //slic3r_params_panel_hpp_
