#ifndef _OBJ_COLOR_DIALOG_H_
#define _OBJ_COLOR_DIALOG_H_

#include "GUI_Utils.hpp"
#include "libslic3r/Color.hpp"
#include <wx/sizer.h>
#include <wx/spinctrl.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/checkbox.h>
#include <wx/msgdlg.h>
#include "Widgets/HoverBorderIcon.hpp"

class Button;
class Label;
class ComboBox;
struct ColorDistValue
{
    int   id;
    float distance;
};

// 用于稳定记录用户在 OBJ 颜色对话框中所做的耗材选择。
// OK 时 add_custom_filament 触发混合耗材重新生成，旧的虚拟 ID 语义变化，
// 因此需要按 (component_a, component_b) / 新增顺序重新计算 paint_color。
enum class ClusterMappingType : unsigned char {
    PHYSICAL     = 0, // 物理 ID = id1
    MIXED_PAIR   = 1, // 混合耗材 = (id1, id2) 组件对
    NEW_PHYSICAL = 2, // 新增物理 = 第 id1 个新物理（1-based 顺序）
};

struct ClusterMapping {
    ClusterMappingType type{ClusterMappingType::PHYSICAL};
    unsigned char      id1{0};
    unsigned char      id2{0};
};
class ObjColorPanel : public wxPanel
{
public:
    // BBS
    ObjColorPanel(wxWindow *                            parent,
                  std::vector<Slic3r::RGBA> &     input_colors,bool  is_single_color,
                  const std::vector<std::string> &      extruder_colours,
                  std::vector<unsigned char> &    filament_ids,
                  unsigned char &                 first_extruder_id);
    void msw_rescale();
    bool is_ok();
    void update_filament_ids();
    struct ButtonState
    {
        ComboBox*   bitmap_combox{nullptr};
        bool      is_map{false};//int id{0};
    };
    void update_warning_text(const wxString& strTxt);

private:
    wxBoxSizer *create_approximate_match_btn_sizer(wxWindow *parent);
    wxBoxSizer *create_add_btn_sizer(wxWindow *parent);
    wxBoxSizer *create_reset_btn_sizer(wxWindow *parent);
    wxBoxSizer *create_extruder_icon_and_rgba_sizer(wxWindow *parent, int id, const wxColour& color);
    std::string get_color_str(const wxColour &color);
    void create_result_button_sizer(wxWindow *parent, int id);
    wxBoxSizer *create_color_icon_and_rgba_sizer(wxWindow *parent, int id, const wxColour& color);
    void update_color_icon_and_rgba_sizer(int id, const wxColour &color);
    ComboBox* CreateEditorCtrl(wxWindow *parent,int id);
    void draw_table();
    void show_sizer(wxSizer *sizer, bool show);
    void redraw_part_table();
    void deal_approximate_match_btn();
    void deal_add_btn();
    void deal_reset_btn();
    void deal_algo(char cluster_number,bool redraw_ui =false);
    void deal_default_strategy();
    // 辅助：基于稳定映射定位 paint_color
    unsigned int find_virtual_id_for_mixed(unsigned char comp_a, unsigned char comp_b, size_t num_physical);
    int          find_mixed_combo_index(unsigned char comp_a, unsigned char comp_b);
    int          find_best_physical_match(const wxColour &cluster_color, int num_physical);
    // combo 位置 <-> ClusterMapping 转换（基于当前 m_colours 和 m_new_add_colors 状态）
    ClusterMapping combo_position_to_mapping(int combo_pos);
    int            mapping_to_combo_position(const ClusterMapping &m);
private:
    //view ui
    wxScrolledWindow *        m_scrolledWindow = nullptr;
    //wxPanel *                 m_page_simple  = nullptr;
    wxBoxSizer *              m_sizer        = nullptr;
    //wxBoxSizer *              m_sizer_simple = nullptr;
    wxTextCtrl *m_color_cluster_num_by_user_ebox{nullptr};
    wxStaticText *             m_warning_text{nullptr};
    Button *    m_quick_approximate_match_btn{nullptr};
    Button *    m_quick_add_btn{nullptr};
    Button *    m_quick_reset_btn{nullptr};
    std::vector<wxButton*> m_extruder_icon_list;
    std::vector<wxButton*> m_color_cluster_icon_list;//need modeify
    std::vector<wxStaticText*> m_color_cluster_text_list;//need modeify
    std::vector<wxGridSizer*> m_row_sizer_list;         // control show or not
    std::vector<ButtonState*> m_result_icon_list;
    int                       m_last_cluster_num{-1};
    const int               m_combox_width{54};
    int                     m_combox_icon_width;
    int                     m_combox_icon_height;
    wxGridSizer*            m_gridsizer = nullptr;
    //data
    char                       m_last_cluster_number{-2};
    std::vector<Slic3r::RGBA>& m_input_colors;
    int m_color_num_recommend{0};
    int m_color_cluster_num_by_algo{0};
    int m_input_colors_size{0};
    std::vector<wxColour> m_colours;//from project and show right
    std::vector<ClusterMapping> m_cluster_mappings;//show middle - 稳定记录每个 cluster 的 paint_color 选择
    std::vector<wxColour> m_cluster_colours;//from_algo and show left
    bool                  m_can_add_filament{true};
    std::vector<wxColour> m_new_add_colors;
    std::vector<bool>     m_color_needs_new_filament;  // 每个颜色是否需要新增耗材
    //algo result
    std::vector<Slic3r::RGBA> m_cluster_colors_from_algo;
    std::vector<int>          m_cluster_labels_from_algo;
    //result
    bool                        m_is_add_filament{false};
    unsigned char&             m_first_extruder_id;
    std::vector<unsigned char> &m_filament_ids;
    wxPanel* m_top_panel = nullptr;
    wxBoxSizer* m_top_panelSizer = nullptr;
    HoverBorderIcon* m_btnMoreColor = nullptr;
    HoverBorderIcon* m_btnLessColor = nullptr;
    wxPanel* m_moreColorPanel = nullptr;
    wxPanel* m_second_panel = nullptr;
    wxBoxSizer* m_second_panelSizer = nullptr;
    bool m_bClickedAddBtn = false;
};

class ObjColorDialog : public Slic3r::GUI::DPIDialog
{
public:
    ObjColorDialog(wxWindow *                         parent,
                   std::vector<Slic3r::RGBA>&   input_colors, bool is_single_color,
                   const std::vector<std::string> &   extruder_colours,
                   std::vector<unsigned char>&        filament_ids,
                   unsigned char &                 first_extruder_id);
    wxBoxSizer* create_btn_sizer(long flags);
    void on_dpi_changed(const wxRect &suggested_rect) override;
private:
    ObjColorPanel*  m_panel_ObjColor  = nullptr;
    std::unordered_map<int, Button *> m_button_list;
    std::vector<unsigned char>&      m_filament_ids;
    unsigned char &                  m_first_extruder_id;
};

#endif  // _WIPE_TOWER_DIALOG_H_