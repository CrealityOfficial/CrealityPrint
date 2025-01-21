#ifndef slic3r_CreatePresetsDialog_hpp_
#define slic3r_CreatePresetsDialog_hpp_

#include "libslic3r/Preset.hpp"
#include "wxExtensions.hpp"
#include "GUI_Utils.hpp"
#include "Widgets/Label.hpp"
#include "Widgets/TextInput.hpp"
#include "Widgets/Button.hpp"
#include "Widgets/RadioBox.hpp"
#include "Widgets/CheckBox.hpp"
#include "Widgets/ComboBox.hpp"
#include "miniz.h"
#include "ParamsDialog.hpp"
#include <nlohmann/json.hpp>

namespace Slic3r { 
namespace GUI {

class SelectPrinterPresetPanel;
class SelectFilamentPresetPanel;

class CreateFilamentPresetDialog : public DPIDialog
{
public:
    CreateFilamentPresetDialog(wxWindow *parent);
    ~CreateFilamentPresetDialog();

protected:
    enum FilamentOptionType { 
        VENDOR = 0,
        TYPE,
        SERIAL,
        FILAMENT_PRESET,
        PRESET_FOR_PRINTER,
        FILAMENT_NAME_COUNT
    };

protected:
    void on_dpi_changed(const wxRect &suggested_rect) override;
    bool        is_check_box_selected();
    wxBoxSizer *create_item(FilamentOptionType option_type, wxWindow* parent = nullptr);
    wxBoxSizer *create_vendor_item();
    wxBoxSizer *create_type_item();
    wxBoxSizer *create_serial_item();
    wxBoxSizer *create_filament_preset_item(wxWindow* parent);
    wxBoxSizer *create_filament_preset_for_printer_item();
    wxBoxSizer *create_button_item();

private:
    void          clear_filament_preset_map();
    wxArrayString get_filament_preset_choices();
    wxBoxSizer *  create_radio_item(wxString title, wxWindow *parent, wxString tooltip, std::vector<std::pair<RadioBox *, wxString>> &radiobox_list);
    void          select_curr_radiobox(std::vector<std::pair<RadioBox *, wxString>> &radiobox_list, int btn_idx);
    wxString      curr_create_filament_type();
    void          get_filament_presets_by_machine();
    void          get_all_filament_presets();
    void          get_all_visible_printer_name();
    void          update_dialog_size();
    template<typename T>
    void          sort_printer_by_nozzle(std::vector<std::pair<std::string, T>> &printer_name_to_filament_preset);

private:
    struct CreateType
    {
        wxString base_filament;
        wxString base_filament_preset;
    };

private:
    std::vector<std::pair<RadioBox *, wxString>>                     m_create_type_btns;
    std::unordered_map<::CheckBox *, std::pair<std::string, Preset *>> m_filament_preset;
    std::unordered_map<::CheckBox *, std::pair<std::string, Preset *>> m_machint_filament_preset;
    std::unordered_map<std::string, std::vector<Preset *>>           m_filament_choice_map;
    std::unordered_map<wxString, std::string>                        m_public_name_to_filament_id_map;
    std::unordered_map<std::string, Preset *>                        m_all_presets_map;
    std::unordered_set<std::string>                                  m_system_filament_types_set;
    std::set<std::string>                                            m_visible_printers;
    CreateType                                                       m_create_type;
    Button *                                                         m_button_create                = nullptr;
    Button *                                                         m_button_cancel                = nullptr;
    ComboBox *                                                       m_filament_vendor_combobox     = nullptr;
    ::CheckBox *                                                     m_can_not_find_vendor_checkbox = nullptr;
    ComboBox *                                                       m_filament_type_combobox       = nullptr;
    ComboBox *                                                       m_exist_vendor_combobox        = nullptr;
    ComboBox *                                                       m_filament_preset_combobox     = nullptr;
    TextInput *                                                      m_filament_custom_vendor_input = nullptr;
    wxGridSizer *                                                    m_filament_presets_sizer       = nullptr;
    wxPanel *                                                        m_filament_preset_panel        = nullptr;
    wxScrolledWindow *                                               m_scrolled_preset_panel        = nullptr;
    TextInput *                                                      m_filament_serial_input        = nullptr;
    wxBoxSizer *                                                     m_scrolled_sizer               = nullptr;
    wxStaticText *                                                   m_filament_preset_text         = nullptr;

};

class CreatePrinterPresetDialog : public DPIDialog
{
public:
    CreatePrinterPresetDialog(wxWindow *parent,int iType = 0);
    ~CreatePrinterPresetDialog();


protected:
       void          set_current_visible_printer_new();

       wxBoxSizer *create_displacement_distance(wxWindow *parent);
       wxBoxSizer *create_height_limit(wxWindow *parent);
       void update_nozzle_data_new();
       void update_nozzle_other_data_new();
       void update_printer_other_data_new();
       wxBoxSizer* create_previous_page_combox(wxWindow *parent);


       void load_nozzle_info_form_json();
       bool load_file(std::string jPath, std::string &sContent);
       int load_profile_family(std::string strVendor, std::string strFilePath);
       void StringReplace(std::string &strBase, std::string strSrc, std::string strDes);
       struct PrinterAndNozzleInfo
       {
           std::string model_name;
           std::string printer_vendor;
           std::vector<std::string> vec_nozzle;
       };
       std::vector<PrinterAndNozzleInfo> m_printer_nozzle_info;
       bool data_init_vendor_and_model();
       void on_preset_model_page2_value_change(wxCommandEvent& e);
       void update_current_printer_text();

       VendorMap m_vendors;
       int m_type;   
       ComboBox* m_printer_vendor_page2 = nullptr;
       ComboBox* m_printer_model_page2 = nullptr;
       float m_nozzle;
       wxStaticText* m_current_printer_text = nullptr;

protected:
    void on_dpi_changed(const wxRect &suggested_rect) override;

/********************************************************    Control Construction    *****************************************************/
    wxBoxSizer *create_step_switch_item();
    //Create Printer Page1
    void        create_printer_page1(wxWindow *parent);
    wxBoxSizer *create_type_item(wxWindow *parent);
    wxBoxSizer *create_printer_item(wxWindow *parent);
    wxBoxSizer *create_nozzle_diameter_item(wxWindow *parent);
    wxBoxSizer *create_bed_shape_item(wxWindow *parent);
    wxBoxSizer *create_bed_size_item(wxWindow *parent);
    wxBoxSizer *create_origin_item(wxWindow *parent);
    wxBoxSizer *create_hot_bed_stl_item(wxWindow *parent);
    wxBoxSizer *create_hot_bed_svg_item(wxWindow *parent);
    wxBoxSizer *create_max_print_height_item(wxWindow *parent);
    wxBoxSizer *create_page1_btns_item(wxWindow *parent);
    //Improt Presets Page2
    void create_printer_page2(wxWindow *parent);
    wxBoxSizer *create_printer_preset_item(wxWindow *parent);
    wxBoxSizer *create_presets_item(wxWindow *parent);
    wxBoxSizer *create_presets_template_item(wxWindow *parent);
    wxBoxSizer *create_page2_btns_item(wxWindow *parent);

    void show_page1();
    void show_page2();

/**********************************************************    Data Interaction    *******************************************************/
    bool          data_init();
    void          set_current_visible_printer();
    void          select_curr_radiobox(std::vector<std::pair<RadioBox *, wxString>> &radiobox_list, int btn_idx);
    void          select_all_preset_template(std::vector<std::pair<::CheckBox *, Preset *>> &preset_templates);
    void          deselect_all_preset_template(std::vector<std::pair<::CheckBox *, Preset *>> &preset_templates);
    void          update_presets_list(bool jast_template = false, bool isUseDefault = true);
    void          on_preset_model_value_change(wxCommandEvent &e);
    void          clear_preset_combobox();
    bool          save_printable_area_config(Preset *preset);
    bool          check_printable_area();
    bool          validate_input_valid();
    void          load_texture();
    void          load_model_stl();
    bool          load_system_and_user_presets_with_curr_model(PresetBundle &temp_preset_bundle, bool just_template = false,bool isUseDefault = true);
    void          generate_process_presets_data(std::vector<Preset const *> presets, std::string nozzle);
    void          update_preset_list_size();
    wxArrayString printer_preset_sort_with_nozzle_diameter(const VendorProfile &vendor_profile, float nozzle_diameter);

    wxBoxSizer *create_radio_item(wxString title, wxWindow *parent, wxString tooltip, std::vector<std::pair<RadioBox *, wxString>> &radiobox_list);

    wxString    curr_create_preset_type();
    wxString    curr_create_printer_type();

private:
    struct CreatePrinterType
    {
        wxString create_printer;
        wxString create_nozzle;
        wxString base_template;
        wxString base_curr_printer;
    };

    CreatePrinterType                                  m_create_type;
    std::vector<std::pair<RadioBox *, wxString>>       m_create_type_btns;    //
    std::vector<std::pair<RadioBox *, wxString>>       m_create_presets_btns; //
    std::vector<std::pair<::CheckBox *, Preset *>>           m_filament_preset;
    std::vector<std::pair<::CheckBox *, Preset *>>           m_process_preset;
    std::unordered_map<std::string, std::shared_ptr<Preset>> m_printer_name_to_preset;
    VendorProfile                                      m_printer_preset_vendor_selected;
    Slic3r::VendorProfile::PrinterModel                m_printer_preset_model_selected;
    bool                                               rewritten                        = false;
    Preset *                                           m_printer_preset                 = nullptr;
    wxStaticBitmap *                                   m_step_1                         = nullptr;
    wxStaticBitmap *                                   m_step_2                         = nullptr;
    Button *                                           m_button_OK                      = nullptr;
    Button *                                           m_button_create                  = nullptr;
    Button *                                           m_button_page1_cancel            = nullptr;
    Button *                                           m_button_page2_cancel            = nullptr;
    Button *                                           m_button_page2_back              = nullptr;
    Button *                                           m_button_bed_stl                 = nullptr;
    Button *                                           m_button_bed_svg                 = nullptr;
    wxScrolledWindow *                                 m_page1                          = nullptr;
    wxPanel *                                          m_page2                          = nullptr;
    wxScrolledWindow *                                 m_scrolled_preset_window         = nullptr;
    wxBoxSizer *                                       m_scrooled_preset_sizer          = nullptr;
    ComboBox *                                         m_select_vendor                  = nullptr;
    ComboBox *                                         m_select_model                   = nullptr;
    ComboBox *                                         m_select_printer                 = nullptr;
    ::CheckBox *                                             m_can_not_find_vendor_combox     = nullptr;
    wxStaticText *                                     m_can_not_find_vendor_text       = nullptr;
    wxTextCtrl *                                       m_custom_vendor_text_ctrl        = nullptr;
    wxTextCtrl *                                       m_custom_model_text_ctrl         = nullptr;
    ComboBox *                                         m_nozzle_diameter                = nullptr;
    ComboBox *                                         m_printer_vendor                 = nullptr;
    ComboBox *                                         m_printer_model                  = nullptr;
    TextInput *                                        m_bed_size_x_input               = nullptr;
    TextInput *                                        m_bed_size_y_input               = nullptr;
    TextInput *                                        m_bed_origin_x_input             = nullptr;
    TextInput *                                        m_bed_origin_y_input             = nullptr;
    TextInput *                                        m_print_height_input             = nullptr;
    wxGridSizer *                                      m_filament_preset_template_sizer = nullptr;
    wxGridSizer *                                      m_process_preset_template_sizer  = nullptr;
    wxPanel *                                          m_filament_preset_panel          = nullptr;
    wxPanel *                                          m_process_preset_panel           = nullptr;
    wxPanel *                                          m_preset_template_panel          = nullptr;
    wxBoxSizer *                                       m_filament_sizer                 = nullptr;
    wxPanel *                                          m_printer_info_panel             = nullptr;
    wxBoxSizer *                                       m_page1_sizer                    = nullptr;
    wxBoxSizer *                                       m_printer_info_sizer             = nullptr;
    wxBoxSizer *                                       m_page2_sizer                    = nullptr;
    wxStaticText *                                     m_upload_stl_tip_text            = nullptr;
    wxStaticText *                                     m_upload_svg_tip_text            = nullptr;
    std::string                                        m_custom_texture;
    std::string                                        m_custom_model;


    wxPanel *                                          m_printer_info_pane_nozzle             = nullptr;
    wxBoxSizer *                                       m_printer_info_sizer_nozzle             = nullptr;
    TextInput*                                         m_print_max_height_input                    = nullptr;
    TextInput*                                         m_print_min_height_input                    = nullptr;
    TextInput*                                         m_print_length_input                        = nullptr;
    
};

enum SuccessType {
    PRINTER = 0,
    FILAMENT
};

class CreatePresetSuccessfulDialog : public DPIDialog
{
public:
    CreatePresetSuccessfulDialog(wxWindow *parent, const SuccessType &create_success_type);
    ~CreatePresetSuccessfulDialog();

protected:
    void on_dpi_changed(const wxRect &suggested_rect) override;

private:
    Button *m_button_ok     = nullptr;
    Button *m_button_cancel = nullptr;
};

class ExportConfigsDialog;
class ExportRightPanel;
class ExportCheckbox : public wxPanel
{
public:
    ExportCheckbox(wxWindow* parent, Preset* preset);
    ::CheckBox* checkbox = nullptr;
    int     getProportion() { return proportion; }
    Preset* preset     = nullptr;
    int     proportion = 1;

private:
    static int s_proportion;
};
class ParamCheckbox : public wxPanel
{
public:
    ParamCheckbox(wxWindow* parent, Preset* preset);
    int     getProportion() { return proportion; }
    Preset* preset     = nullptr;
    int     proportion = 1;
    void*   data       = nullptr;

private:
    static int s_proportion;
};

class HoveableCheckbox;
class ExportMidPanel : public wxPanel
{
public:
    struct STLineDataNode
    {
        // Preset* preset = nullptr;
        std::string        name = "";
        std::list<Preset*> lstPrinterPresetParam;
        std::list<Preset*> lstFilamentPresetParam;
        std::list<Preset*> lstProcessPresetParam;
    };
    struct STTreeLineDataNode
    {
        wxBoxSizer*                sizer    = nullptr;
        HoveableCheckbox*          checkbox = nullptr;
        wxString                   name     = "";
        std::list<ExportCheckbox*> lstPrinterPresetParam;
        std::list<ExportCheckbox*> lstFilamentPresetParam;
        std::list<ExportCheckbox*> lstProcessPresetParam;
        bool                       hasCreateCtl = false;
        STLineDataNode             data;
    };
    struct STTreeDataNode
    {
        ~STTreeDataNode()
        {
            for (auto item : lstTreeLineDataNode) {
                delete item;
                item = nullptr;
            }
            lstTreeLineDataNode.clear();
        }
        wxBoxSizer*                    sizer    = nullptr;
        wxPanel*                       panel    = nullptr;
        ::CheckBox*                    checkbox = nullptr;
        wxString                       name     = "";
        bool                           isExpanded = true;
        std::list<STTreeLineDataNode*> lstTreeLineDataNode;
    };
    struct STPresets
    {
        ~STPresets()
        {
            for (auto item : lstPresets) {
                delete item;
                item = nullptr;
            }
            lstPresets.clear();
        }
        bool                       hasInit = false;
        std::list<STTreeDataNode*> lstPresets;
        STTreeLineDataNode*        selectedLineDataNode = nullptr;
    };
    ExportMidPanel(wxWindow* parent);
    ~ExportMidPanel();

    bool hasPrinterInit();
    bool hasFilamentInit();
    bool hasProcessInit();
    void updatePrinterPresets(const std::unordered_map<std::string, std::list<STLineDataNode*>>& mapPrinterPresets);
    void updateFilamentPresets(const std::unordered_map<std::string, std::list<STLineDataNode*>>& mapFilamentPresets);
    void updateProcessPresets(const std::unordered_map<std::string, std::list<STLineDataNode*>>& mapProcessPresets);
    void              getCheckedPrinterPresets(std::list<std::shared_ptr<STLineDataNode>>& lstCheckedPrinterPresets);
    void              getCheckedFilamentPresets(std::list<std::shared_ptr<STLineDataNode>>& lstCheckedFilamentPresets);
    void              getCheckedProcessPresets(std::list<std::shared_ptr<STLineDataNode>>& lstCheckedProcessPresets);
    void              funcRightPanelItemCheckedCb(bool value);
    ExportRightPanel*    m_rightPanel = nullptr;

private:
    STTreeDataNode*     createTreeNode(wxString nodeName);
    STTreeLineDataNode* createCheckbox(const STLineDataNode* pSTLineDataNode);
    void                updateCtrlSize();

private:
    struct STTreeNode
    {
        wxBoxSizer* mainBoxSize = nullptr;
    };
    STTreeNode                             m_stPrinterParam;
    STTreeNode                             m_stFilamentParam;
    STTreeNode                             m_stProcessParam;
    STPresets                              m_stPrinterPresets;
    STPresets                              m_stFilamentPresets;
    STPresets                              m_stProcessPresets;
    wxStaticText*                          m_pSttText        = nullptr;
    wxBoxSizer*                            m_bkBoxSizer      = nullptr;
    wxScrolledWindow*                      m_scrolledWindow  = nullptr;
    wxBoxSizer*                            m_mainBoxSizer    = nullptr;
    bool                                   m_isAllShowed     = true;
    std::function<void(wxString itemName)> m_funcItemClicked = nullptr;
    std::function<void(bool)>              m_funcItemChecked = nullptr;
    std::map<std::string, STTreeNode*>     m_mapFilamentPresetsCtrl;
    int                                    m_index = -1;
};

class ExportRightPanel : public wxPanel
{
public:
    ExportRightPanel(wxWindow* parent, wxString titleName);

    void updatePresets(ExportMidPanel::STTreeLineDataNode* pLineDataNode);
    void setExportBtnClicked(std::function<void()> funcExportBtnClicked);
    void setCancelBtnClicked(std::function<void()> funcCancelBtnClicked);
    void setCheckboxClickedCb(std::function<void(bool)> funcCheckboxClickedCb);
    void setExportBtnState(bool bCanExport);

    wxScrolledWindow*    m_scrolledWindow  = nullptr;
private:
    wxBoxSizer*          m_mainBoxSizer    = nullptr;
    wxBoxSizer*          m_midDataBoxSizer = nullptr;
    struct STListNode
    {
        wxPanel*    panel          = nullptr;
        wxBoxSizer* mainBoxSize    = nullptr;
        wxBoxSizer* contentBoxSize = nullptr;
    };
    STListNode                                       m_stPrinterPresetParam;
    STListNode                                       m_stFilamentPresetParam;
    STListNode                                       m_stProcessPresetParam;
    std::function<void()>                            m_funcExportBtnClicked = nullptr;
    std::function<void()>                            m_funcCancleBtnClicked = nullptr;
    wxButton*                                        m_pBtnExport           = nullptr;
    wxButton*                                        m_pBtnCalcel           = nullptr;
    bool                                             m_bCanExport           = false;
    std::function<void(bool)>                            m_funcCheckboxClickedCb  = nullptr;
};

class ExportConfigsDialog : public DPIDialog
{
public:
    ExportConfigsDialog(wxWindow* parent, int type);
    ExportConfigsDialog(wxWindow* parent, const wxString& export_type = "");
    ~ExportConfigsDialog();//to do: delete preset

    void exportPresets();

protected:
    
    struct ExportType
    {
        wxString preset_bundle;
        wxString filament_bundle;
        wxString printer_preset;
        wxString filament_preset;
        wxString process_preset;
    };

    enum ExportCase {
        INITIALIZE_FAIL = 0,
        ADD_FILE_FAIL,
        ADD_BUNDLE_STRUCTURE_FAIL,
        FINALIZE_FAIL,
        OPEN_ZIP_WRITTEN_FILE,
        EXPORT_CANCEL,
        EXPORT_SUCCESS,
        CASE_COUNT,
    };

private:
    void        data_init();
    void        set_export_type(const wxString& export_type);
    void        select_curr_radiobox(std::vector<std::pair<RadioBox*, wxString>>& radiobox_list, int btn_idx);
    void        on_dpi_changed(const wxRect &suggested_rect) override;
    void        show_export_result(const ExportCase &export_case);
    bool        has_check_box_selected();
    bool        earse_preset_fields_for_safe(Preset *preset);
    std::string initial_file_path(const wxString &path, const std::string &sub_file_path);
    std::string initial_file_name(const wxString &path, const std::string file_name);
    wxBoxSizer *create_export_config_item(wxWindow *parent);
    wxBoxSizer *create_button_item(wxWindow *parent);
    wxBoxSizer *create_select_printer(wxWindow *parent);
    wxBoxSizer *create_radio_item(wxString title, wxWindow *parent, wxString tooltip, std::vector<std::pair<RadioBox *, wxString>> &radiobox_list);
    int         initial_zip_archive(mz_zip_archive &zip_archive, const std::string &file_path);
    ExportCase  save_zip_archive_to_file(mz_zip_archive &zip_archive);
    ExportCase  save_presets_to_zip(const std::string &export_file, const std::vector<std::pair<std::string, std::string>> &config_paths);
    ExportCase  archive_preset_bundle_to_file(const wxString &path);
    ExportCase  archive_filament_bundle_to_file(const wxString &path);
    ExportCase  archive_printer_preset_to_file(const wxString &path);
    ExportCase  archive_filament_preset_to_file(const wxString &path);
    ExportCase  archive_process_preset_to_file(const wxString &path);

private:
    wxBoxSizer* create_left_navigation(wxWindow* parent);
    wxBoxSizer* createRightContent(wxWindow* parent);
    wxBoxSizer* createMidPresets(wxWindow* parent);
    wxBoxSizer* create_right_export_context(wxWindow* parent);
    void        onBtnPrinterClicked();
    void        onBtnFilamentClicked();
    void        onBtnProcessClicked();
    void        cancelExport();
    ExportCase  archivePresetToFile();

private:
    struct STPresets
    {
        wxButton* pBtnPresets = nullptr;
        bool      bClicked           = false;
    };
    STPresets                  m_stPrinterPresets;
    STPresets                  m_stFilamentPresets;
    STPresets                  m_stProcessPresets;
    ExportMidPanel*            m_exportMidPanel      = nullptr;
    ExportRightPanel*          m_exportRightPanel    = nullptr;
    int                        m_index               = -1;

private:
    std::vector<std::pair<RadioBox *, wxString>>           m_export_type_btns;
    std::vector<std::pair<::CheckBox *, Preset *>>         m_preset;         // for printer preset bundle,printer preset, process preset export
    std::vector<std::pair<::CheckBox *, std::string>>      m_printer_name;    // for filament and peocess preset export, collaborate with m_filament_name_to_presets
    std::unordered_map<std::string, Preset *>              m_printer_presets;//first: printer name, second: printer presets have same printer name
    std::unordered_map<std::string, std::vector<Preset *>> m_filament_presets;//first: printer name, second: filament presets have same printer name
    std::unordered_map<std::string, std::vector<Preset *>> m_process_presets;//first: printer name, second: filament presets have same printer name
    std::unordered_map<std::string, std::vector<std::pair<std::string, Preset *>>> m_filament_name_to_presets;//first: filament name, second presets have same filament name and printer name in vector
    ExportType                                             m_exprot_type;
    wxString                                               m_cur_export_type;
    wxBoxSizer *                                           m_main_sizer             = nullptr;
    wxScrolledWindow *                                     m_scrolled_preset_window = nullptr;
    wxGridSizer *                                          m_preset_sizer   = nullptr;
    wxPanel *                                              m_presets_window = nullptr;
    Button *                                               m_button_ok      = nullptr;
    Button *                                               m_button_cancel  = nullptr;
    wxStaticText *                                         m_serial_text    = nullptr;
};

class CreatePresetForPrinterDialog : public DPIDialog
{
public:
    CreatePresetForPrinterDialog(wxWindow *parent, std::string filament_type, std::string filament_id, std::string filament_vendor, std::string filament_name);
    ~CreatePresetForPrinterDialog();

private:
    void        on_dpi_changed(const wxRect &suggested_rect) override;
    void        get_visible_printer_and_compatible_filament_presets();
    wxBoxSizer *create_selected_printer_preset_sizer();
    wxBoxSizer *create_selected_filament_preset_sizer();
    wxBoxSizer *create_button_sizer();

private:
    std::string                                                                       m_filament_id;
    std::string                                                                       m_filament_name;
    std::string                                                                       m_filament_vendor;
    std::string                                                                       m_filament_type;
    std::shared_ptr<PresetBundle>                                                     m_preset_bundle;
    ComboBox *                                                                        m_selected_printer  = nullptr;
    ComboBox *                                                                        m_selected_filament = nullptr;
    Button *                                                                          m_ok_btn            = nullptr;
    Button *                                                                          m_cancel_btn        = nullptr;
    std::unordered_map<wxString, std::shared_ptr<Preset>>                             filament_choice_to_filament_preset;
    std::unordered_map<std::string, std::vector<std::shared_ptr<Preset>>>             m_printer_compatible_filament_presets; // need be used when add presets

};

class EditFilamentPresetDialog;

class PresetTree
{
public:
    PresetTree(EditFilamentPresetDialog *dialog);
    
    wxPanel *get_preset_tree(std::pair<std::string, std::vector<std::shared_ptr<Preset>>> printer_and_presets);

private:
    wxPanel *get_root_item(wxPanel *parent, const std::string &printer_name);

    wxPanel *get_child_item(wxPanel *parent, std::shared_ptr<Preset> preset, std::string printer_name, int preset_index, bool is_last = false);

    void delete_preset(std::string printer_name, int need_delete_preset_index);

    void edit_preset(std::string printer_name, int need_edit_preset_index);

private:
    EditFilamentPresetDialog *                                   m_parent_dialog = nullptr;
    std::pair<std::string, std::vector<std::shared_ptr<Preset>>> m_printer_and_presets;

};

class EditFilamentPresetDialog : public DPIDialog
{
public:
    EditFilamentPresetDialog(wxWindow *parent, FilamentInfomation *filament_info);
    ~EditFilamentPresetDialog();
    
    wxPanel *get_preset_tree_panel() { return m_preset_tree_panel; }
    std::shared_ptr<Preset> get_need_edit_preset() { return m_need_edit_preset; }
    void     set_printer_name(const std::string &printer_name) { m_selected_printer = printer_name; }
    void     set_need_delete_preset_index(int need_delete_preset_index) { m_need_delete_preset_index = need_delete_preset_index; }
    void     set_need_edit_preset_index(int need_edit_preset_index) { m_need_edit_preset_index = need_edit_preset_index; }
    void     delete_preset();
    void     edit_preset();

private:
    void        on_dpi_changed(const wxRect &suggested_rect) override;
    bool        get_same_filament_id_presets(std::string filament_id);
    void        update_preset_tree();
    wxBoxSizer *create_filament_basic_info();
    wxBoxSizer *create_add_filament_btn();
    wxBoxSizer *create_preset_tree_sizer();
    wxBoxSizer *create_button_sizer();

private:
    PresetTree *                                                          m_preset_tree_creater = nullptr;
    std::string                                                           m_filament_id;
    std::string                                                           m_filament_name;
    std::string                                                           m_vendor_name;
    std::string                                                           m_filament_type;
    std::string                                                           m_filament_serial;
    Button *                                                              m_add_filament_btn         = nullptr;
    Button *                                                              m_del_filament_btn         = nullptr;
    Button *                                                              m_ok_btn                   = nullptr;
    wxBoxSizer *                                                          m_preset_tree_sizer        = nullptr;
    wxPanel *                                                             m_preset_tree_panel        = nullptr;
    wxScrolledWindow *                                                    m_preset_tree_window       = nullptr;
    wxBoxSizer *                                                          m_main_sizer               = nullptr;
    wxStaticText *                                                        m_note_text                = nullptr;
    int                                                                   m_need_delete_preset_index = -1;
    int                                                                   m_need_edit_preset_index   = -1;
    std::shared_ptr<Preset>                                               m_need_edit_preset;
    std::string                                                           m_selected_printer         = "";
    std::unordered_map<std::string, std::vector<std::shared_ptr<Preset>>> m_printer_compatible_presets;

};

class SelectPrinterPresetDialog : public DPIDialog
{
public:
    SelectPrinterPresetDialog(nlohmann::json& profile_json, wxWindow* parent);
    ~SelectPrinterPresetDialog();

    void on_dpi_changed(const wxRect& suggested_rect) override;

private:
    SelectPrinterPresetPanel* m_panel = nullptr;
};

}
}
#endif