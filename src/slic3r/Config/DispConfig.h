#ifndef slic3r_GUI_DispConfig1
#define slic3r_GUI_DispConfig1

#include "imgui/imgui.h"
#include "functional"

namespace Slic3r {

namespace GUI {

    class GLTexture;

    class DispConfig
    {
    public:
        void setDarkMode(bool dark);

        enum TextureType
        {
            e_tt_home = 0,
            e_tt_bed_custom,
            e_tt_bed_smooth,
            e_tt_bed_texture,
            e_tt_edit,
            e_tt_close,
            e_tt_layout,
            e_tt_pickbot,
            e_tt_unlock,
            e_tt_setting,
            e_tt_lock,
            e_tt_setting_dirty,
            e_tt_cancle,
            e_tt_error,
            e_tt_warning,
            e_tt_normal,
            e_tt_layer,
            e_tt_collapse,
            e_tt_collapse_item,
            e_tt_gcode_play,
            e_tt_gcode_pause,
            e_tt_rounding_transparent,
            e_tt_retangle_transparent,
            e_tt_count
        };
       
        /** texture path name example
         *  have dark/light,have selected,have hover 
         *      path/to/image/dark/select_hover_name.svg
         *      path/to/image/light/unselect_none_name.svg
         *  no dark/light,no selected,have hover
         *      path/to/image/hover_name.png
         *      path/to/image/none_name.png
         */        
        GLTexture* getTexture(TextureType, bool hover = false, bool sel = false);
        void* getTextureId(TextureType, bool hover = false, bool sel = false);

        enum ColorType {
            e_ct_none,
            e_ct_white,
            e_ct_btnBg,
            e_ct_btnBgSelWhite,
            e_ct_btnBgSel,
            e_ct_text,
            e_ct_hyperText,
            e_ct_errorText,
            e_ct_slider,
            e_ct_slider_hand,
            e_ct_sliderSel,
            e_ct_sliderTip,
            e_ct_inter,
            e_ct_interSel,
            e_ct_line,
            e_ct_lineSel,
        };
        ImVec4 getColor(ColorType);
        ImU32 getColorImU32(ColorType);

        enum WindowType
        {
            e_wt_slice = 0
            ,e_wt_slice_list
            ,e_wt_print
            ,e_wt_print_list
            ,e_wt_slider  //vertical, for cliper slider
            ,e_wt_slider_move   //horizontal, for moves slider
            ,e_wt_slider_gcode  //vertical, for layers slider
            ,e_wt_msg
            ,e_wt_error
            ,e_wt_reset
            ,e_wt_gcode
        };
        void prepareWindow(WindowType, ImVec2 ref, float scale = 1);
        ImVec2 getWindowSize(WindowType, float scale = 1);
        ImVec2 getWindowBias(WindowType, float scale = 1);

        struct WindowConfig
        {
            WindowConfig() noexcept{};
            float round = 5.0;
            float bgalpha = 0.1;
            float border = 0.0;
            bool notitle = true;
            ColorType txt = e_ct_text;
            ImVec2 space{ -1,-1 };
            ImVec2 padding{ 10,10 };
        };
        using CreateFn = std::function<void()>;
        void processWindows(const wxString& name, CreateFn, WindowConfig = {});

        struct ButtonConfig
        {
            ButtonConfig() noexcept{};
            ColorType fg = DispConfig::e_ct_btnBgSel;
            ColorType bg = e_ct_none;
            ImVec2 size = {0,0};
            float border = 1;
            float boldScale = 1;
            wxString tips = {};
        };
        /**
         * @param type[in]    button enable
         *                    0    enable ,1   disable ,2   always enable
         */
        bool normalButton(const wxString& name, ButtonConfig, int type = 0);
        int popupButton(const wxString& name, ButtonConfig bcfg,const std::vector< wxString>& list,ButtonConfig pcfg);
        bool checkBox(const wxString& name, bool check,ButtonConfig = {});

        struct ComboConfig
        {
            ComboConfig() noexcept{};
            float bgalpha = 0.6;
            ColorType hgc = e_ct_btnBgSel;
        };
        int combo(const std::vector<std::string>& list, int current,ComboConfig = {});
        
        ImVec2 boldText(const std::string&,float scale = 1);
    };

}}
#endif
