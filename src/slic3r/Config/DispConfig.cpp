#include "DispConfig.h"
#include "../GUI/GLTexture.hpp"
#include "../GUI/GUI_App.hpp"
#include "libslic3r/Utils.hpp"

namespace Slic3r {

namespace GUI {

static bool s_isDark = true;
static GLTexture* s_texture[DispConfig::e_tt_count][4] = {nullptr};

static int Loc_roundStyle(float rd)
{
    if (rd < 0)
        return 0;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, rd);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, rd);
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, rd);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, rd);
    return 4;
}

static int Loc_borderStyle(float bd)
{
    if (bd < 0)
        return 0;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, bd);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, bd);
    ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, bd);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, bd);
    return 4;
}

static int Loc_padingStyle(ImVec2 bd)
{
    if (bd.x < 0 || bd.y < 0)
        return 0;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, bd);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0,0));
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, bd);
    return 3;
}

static int Loc_spaceStyle(ImVec2 bd)
{
    if (bd.x < 0 || bd.y < 0)
        return 0;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, bd);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, bd);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, bd);
    ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, bd);
    return 4;
}

static int Loc_bgStyle(float alpha) {
    ImVec4 bgc = { 0,0,0,alpha };
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(.0f, .0f, .0f, 0.f));
    ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(1.0f, 1.0f, 1.0f, 0.6f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 1.0f, 1.0f, 0.6f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, bgc);
    ImGui::PushStyleColor(ImGuiCol_PopupBg, bgc);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, bgc);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, bgc);
    return 7;
}

static void Loc_pushBoldStyle(float scale) {
    ImGuiWrapper& imgui = *wxGetApp().imgui();
    if (scale>1)imgui.push_bold_font();
    ImGui::SetWindowFontScale(scale);
}

static void Loc_popBoldStyle(float scale) {
    ImGuiWrapper& imgui = *wxGetApp().imgui();
    ImGui::SetWindowFontScale(1);
    if (scale > 1)imgui.pop_bold_font();
}

void DispConfig::setDarkMode(bool dark) 
{ 
    if (s_isDark!=dark)
    {
        for (auto &txu:s_texture)
        {
            for (auto &tx:txu)
            {
                if (tx)
                    delete tx;
                tx = nullptr;
            }
        }
    }
    s_isDark = dark;
}

void* DispConfig::getTextureId(TextureType tt, bool hover, bool sel) {
    auto tex = getTexture(tt,hover,sel);
    if (tex)
    {
        return (void*)tex->get_id();
    }
    return nullptr;
}

GLTexture* DispConfig::getTexture(TextureType tt, bool hover, bool sel) {
    auto& tex = s_texture[tt][(sel?1:0)+(hover?2:0)];
    if (tex ==nullptr)
    {
        //tuple0:base name
        //tuple1:needDarkLight
        //tuple2:needHover
        //tuple3:needSel
        static std::map<TextureType, std::tuple<std::string,bool,bool,bool>> s_names = {
            {e_tt_home,{"home.png",true,true,false}},
            {e_tt_collapse,{"collapse_bk.png",true,false,false}},
            {e_tt_collapse_item,{"collapse.png",true,true,true}},
            {e_tt_bed_custom,{"custom.png",false,false,false}},
            {e_tt_bed_texture,{"texture.png",false,false,false}},
            {e_tt_bed_smooth,{"smooth.svg",false,false,false}},
            {e_tt_layer,{"layer.svg",false,true,false}},
            {e_tt_edit,{"edit.png",true,true,true}},
            {e_tt_close,{"close.png",true,true,true}},
            {e_tt_layout,{"autolayout.png",true,true,true}},
            {e_tt_pickbot,{"pickbottom.png",true,true,true}},
            {e_tt_unlock,{"unlock.png",true,true,true}},
            {e_tt_setting,{"setting.png",true,true,true}},
            {e_tt_setting_dirty, {"setting_dirty.png", true, true, true}},
            {e_tt_lock,{"lock.png",true,true,true}},
            {e_tt_cancle,{"cancel.svg",true,false,false}},
            {e_tt_error,{"error_message.svg",false,false,false}},
            {e_tt_warning,{"warning_message.svg",false,false,false}},
            {e_tt_normal,{"normal_message.svg",false,false,false}},
            {e_tt_gcode_play, {"gcode_preview_play.svg", false, false, false}},
            {e_tt_gcode_pause, {"gcode_preview_pause.svg", false, false, false}},
            {e_tt_rounding_transparent, {"rounding_transparent.svg", false, false, false}},
            {e_tt_retangle_transparent, {"retangle_transparent.svg", false, false, false}},
        };
        std::string path = resources_dir() + "/images/";
        const auto &[name,needDark,needHover,needSel] = s_names[tt];
        if (needDark)
            path += s_isDark ? "dark/" : "light/";
        if (needSel)
            path += sel ? "select_" : "unselect_";
        if (needHover)
            path += hover ? "hover_" : "none_";
        path += name;
        tex = new GLTexture();
        tex->load_from_png_svg_file(path);
    }
    return tex;
}

bool DispConfig::checkBox(const wxString& name, bool check,ButtonConfig cfg) {
    auto bg = getColor(check? cfg.fg:cfg.bg);
    auto fg = getColor(cfg.fg);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.0f);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, bg);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,fg);
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, fg);
    ImGui::PushStyleColor(ImGuiCol_BorderActive, fg);
    ImGui::PushStyleColor(ImGuiCol_Border, s_isDark ? ImVec4(122 / 255.f, 122 / 255.f, 127 /255.f, 1) :  ImVec4(214 / 255.f, 214 / 255.f, 220 / 255.f, 1));
    ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(1, 1, 1, 1));

    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, cfg.border);
    Loc_pushBoldStyle(cfg.boldScale);
    ImGui::BBLCheckbox(name.c_str(), &check);
    Loc_popBoldStyle(cfg.boldScale);
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(6);

    return check;
}

bool DispConfig::normalButton(const wxString& name, ButtonConfig cfg,int en) {
    auto bg = getColor(en == 2 ? cfg.fg : cfg.bg);
    auto fg = getColor(en == 1 ? cfg.bg : cfg.fg);

    ImGui::PushStyleColor(ImGuiCol_Button, bg);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, fg);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, fg);
    if (en==2)
        ImGui::PushStyleColor(ImGuiCol_Border, fg);
    else
        ImGui::PushStyleColor(ImGuiCol_BorderActive, fg);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, cfg.border);
    Loc_pushBoldStyle(cfg.boldScale);
    bool active = ImGui::BBLButton(name.ToUTF8(), cfg.size);
    Loc_popBoldStyle(cfg.boldScale);
    ImGui::PopStyleVar(1);
    ImGui::PopStyleColor(4);
    if (!cfg.tips.empty()&&ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::TextUnformatted(cfg.tips.c_str());
        ImGui::EndTooltip();
    }
    return en == 1 ? false : active;
}

int DispConfig::combo(const std::vector<std::string>& list,int current, ComboConfig cfg) {

    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0,4));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0,0,0,0));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));

    ImVec4 bgc(0.0f, 0.0f, 0.0f, 0.9f);
    ImGui::PushStyleColor(ImGuiCol_PopupBg, s_isDark ? bgc : ImVec4(1,1,1,1));

    ImVec4 hgc = getColor(cfg.hgc);
    ImGui::PushStyleColor(ImGuiCol_BorderActive, hgc);
    ImGui::PushStyleColor(ImGuiCol_Border, s_isDark ? ImVec4(108 / 255.f, 108 / 255.f, 112 / 255.f, 1) : ImVec4(214 / 255.f, 214 / 255.f, 220 /255.f,1));
    ImGui::PushStyleColor(ImGuiCol_Header, hgc);
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, hgc);
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, hgc);

    int ret = -1;
    if (ImGui::BBLBeginCombo("", list[current].c_str(), ImGuiComboFlags_HeightLargest))
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
        for (int i = 0; i < list.size(); i++) {
            const bool is_selected = (current == i);
            if (ImGui::BBLSelectable(list[i].c_str(), is_selected)) {
                ret = i;
            }
            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::PopStyleVar(3);
        ImGui::EndCombo();
    }
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(10);
    return ret;
}

ImVec2 DispConfig::getWindowSize(WindowType tp, float scale) {
    ImVec2 winsize(0,0);
    switch (tp) {
    case DispConfig::e_wt_gcode:
        winsize = { 300, -1 };
        break;
    case DispConfig::e_wt_reset:
        winsize = { 20, 17 };
        break;
    case DispConfig::e_wt_slice:
        winsize = { 215, 55 };
        break;
    case DispConfig::e_wt_msg:
        winsize = { 255, 85 };
        break;
    case DispConfig::e_wt_error:
        winsize = { 405, 85 };
        break;
    case DispConfig::e_wt_slice_list:
        winsize = { 215, 95 };
        break;
    case DispConfig::e_wt_print:
        winsize = { 215, 55 };
        break;
    case DispConfig::e_wt_print_list:
        winsize = { 215, 175 };
        break;
    case DispConfig::e_wt_slider:
        winsize = { 50,400 };
        break;
    case DispConfig::e_wt_slider_move:
        winsize = { 460,50 };
        break;
    case DispConfig::e_wt_slider_gcode:
        winsize = {100, 400};
        break;
    default: break;
    }
    winsize.x *= scale;
    winsize.y *= scale;
    return winsize;
}

ImVec2 DispConfig::getWindowBias(WindowType tp, float scale) {
    ImVec2 bias(0,0);
    switch (tp) {
    case DispConfig::e_wt_gcode:
        bias = { 90, 10 };
        break;
    case DispConfig::e_wt_reset:
        bias = { 0, 0 };
        break;
    case DispConfig::e_wt_slice:
        bias = { -10, -60 };
        break;
    case DispConfig::e_wt_msg:
    case DispConfig::e_wt_error:
        bias = { 5, -5 };
        break;
    case DispConfig::e_wt_slice_list:
        bias = { -10, -115 };
        break;
    case DispConfig::e_wt_print:
        bias = { -10, -5 };
        break;
    case DispConfig::e_wt_print_list:
        bias = { -10, -58 };
        break;
    case DispConfig::e_wt_slider:
        bias = { -70,165 };
        break;
    case DispConfig::e_wt_slider_move:
        bias = { 0,-10 };
        break;
    default: break;
    }
    bias.x *= scale;
    bias.y *= scale;
    return bias;
}

void DispConfig::prepareWindow(WindowType tp,ImVec2 ref,float scale){
     ImVec2 bias = getWindowBias(tp, scale);
     ImVec2 winsize = getWindowSize(tp,scale);
     ImVec2 privot(0,0);
     if (abs(bias.x) < 0.0000001)
         privot.x = 0.5;
     else if (bias.x < 0)
         privot.x = 1;
     if (abs(bias.y) < 0.0000001)
         privot.y = 0.5;
     else if (bias.y < 0)
         privot.y = 1;
     ref.x += bias.x;
     ref.y += bias.y;
     ImGui::SetNextWindowPos(ref, ImGuiCond_Always, privot);
     ImGui::SetNextWindowSize(winsize);
 }

void DispConfig::processWindows(const wxString& name,CreateFn fn, WindowConfig st){
    
    int varnum = 0;
    varnum += Loc_roundStyle(st.round); 
    varnum += Loc_borderStyle(st.border);
    varnum += Loc_spaceStyle(st.space);
    varnum += Loc_padingStyle(st.padding);

    int colornum = 1;
    ImGui::PushStyleColor(ImGuiCol_Text, DispConfig().getColor(st.txt));
    colornum += Loc_bgStyle(st.bgalpha);

    if (name == "gcode_legend") {
        if(!s_isDark)
        {
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4{ 255 / 255.f,255 / 255.f,255 / 255.f,1.0f });
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{ 51 / 255.f,51 / 255.f,51 / 255.f,1.f });
            colornum += 2;
        }
        
    }
    int flag = 0;
    if (st.notitle)
        flag = ImGuiWindowFlags_NoCollapse
        | ImGuiWindowFlags_NoTitleBar
        | ImGuiWindowFlags_NoMove
        | ImGuiWindowFlags_NoResize
        | ImGuiWindowFlags_NoScrollbar 
        | ImGuiWindowFlags_NoScrollWithMouse 
        | ImGuiWindowFlags_NoBringToFrontOnFocus;
    if (ImGui::Begin(name.c_str(), nullptr, flag))
        if (fn)
            fn();
    ImGui::End();
    ImGui::PopStyleColor(colornum);
    ImGui::PopStyleVar(varnum);
}

int DispConfig::popupButton(const wxString& name, ButtonConfig bcfg
    , const std::vector< wxString>& list, ButtonConfig cfg) {
    if (normalButton(name,bcfg))
        ImGui::OpenPopup(name.ToUTF8());
    int ret = -1;
    if (ImGui::BeginPopup(name.ToUTF8()))
    {
        auto bg = getColor(cfg.bg);
        auto fg = getColor(cfg.fg);
        ImGui::PushStyleColor(ImGuiCol_Button, bg);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, fg);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, fg);
        for (int i=0;i<list.size();++i)
        {
            if (ImGui::Button(list[i].ToUTF8(), cfg.size))
                ret = i;    
        }
        if (ret>=0)
            ImGui::CloseCurrentPopup();
        ImGui::PopStyleColor(3);
        ImGui::EndPopup();
    }
    return ret;
}

ImVec2 DispConfig::boldText(const std::string&str,float scale) {
    Loc_pushBoldStyle(scale);
    auto sz = ImGui::CalcTextSize(str.c_str());
    ImGui::Text(str.c_str());
    Loc_popBoldStyle(scale);
    return sz;
}

ImU32 DispConfig::getColorImU32(ColorType ct) {
    ImU32 ret = 0; ;
    switch (ct) {
    case Slic3r::GUI::DispConfig::e_ct_white:
        ret = IM_COL32(255, 255, 255, 255);
        break;
    case Slic3r::GUI::DispConfig::e_ct_none:
        ret = IM_COL32(0, 0, 0, 0);
        break;
    case Slic3r::GUI::DispConfig::e_ct_errorText:
        ret = IM_COL32(255, 0, 0, 255);
        break;
    case Slic3r::GUI::DispConfig::e_ct_hyperText:
        ret = IM_COL32(23, 204, 95, 255);
        break;
    case Slic3r::GUI::DispConfig::e_ct_line:
        ret = s_isDark ? IM_COL32(67, 67, 70, 255) : IM_COL32(219, 219, 228, 255);
        break;
    case Slic3r::GUI::DispConfig::e_ct_lineSel:
        ret = s_isDark ? IM_COL32(126, 126, 132, 255) : IM_COL32(205, 205, 211, 255);
        break;
    case Slic3r::GUI::DispConfig::e_ct_btnBg:
        ret = s_isDark ? IM_COL32(89, 89, 93,255) : IM_COL32(255, 255, 255,255);
        break;
    case Slic3r::GUI::DispConfig::e_ct_inter:
        ret = s_isDark ? IM_COL32(62, 62, 64, 255) : IM_COL32(234,234,238, 255);
        break;
    case Slic3r::GUI::DispConfig::e_ct_interSel:
        ret = s_isDark ? IM_COL32(99, 99, 103, 255) : IM_COL32(119,119,125, 255);
        break;
    case Slic3r::GUI::DispConfig::e_ct_btnBgSelWhite:
        ret = s_isDark ? IM_COL32(110, 110, 115, 255) : IM_COL32(242, 242, 245,255);
        break;
    case Slic3r::GUI::DispConfig::e_ct_btnBgSel:
        ret = IM_COL32(23, 204, 95, 255);
        break;
    case Slic3r::GUI::DispConfig::e_ct_text:
        ret = s_isDark ? IM_COL32(255, 255, 255, 255) : IM_COL32(51, 51, 51,255);
        break;
    case Slic3r::GUI::DispConfig::e_ct_slider:
        ret = s_isDark ? IM_COL32(74, 74, 77,255) : IM_COL32(206, 206, 207,255);
        break;
    case Slic3r::GUI::DispConfig::e_ct_sliderSel:
        ret = IM_COL32(23, 204, 95,255);
        break;
    case Slic3r::GUI::DispConfig::e_ct_slider_hand:
        ret = IM_COL32(0, 255, 101,255);
        break;
    case Slic3r::GUI::DispConfig::e_ct_sliderTip:
        ret = s_isDark ? IM_COL32(66, 75, 81,255) : IM_COL32(242, 242, 245,255);
        break;
    default: break;
    }
    return ret;
}

ImVec4 DispConfig::getColor(ColorType ct) {
    auto u32 = getColorImU32(ct);
    ImColor color = ImColor(u32);
    return color.Value;
}

}}