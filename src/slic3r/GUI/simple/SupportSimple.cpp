#include "SupportSimple.hpp"

#include "slic3r/GUI/GLCanvas3D.hpp"
#include "slic3r/GUI/ImGuiWrapper.hpp"
#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/Tab.hpp"
#include "slic3r/GUI/I18N.hpp"
#include "slic3r/GUI/Plater.hpp"
#include "libslic3r/PrintConfig.hpp"

#include <imgui/imgui.h>

namespace Slic3r { namespace GUI { namespace SupportSimple {

void render_simple_input_window()
{
    const float scale = wxGetApp().plater()->get_current_canvas3D()->get_scale();

    // Read global (print preset) config, regardless of model selection
    const DynamicPrintConfig& glb_cfg = wxGetApp().preset_bundle->prints.get_edited_preset().config;
    bool enable_support = glb_cfg.opt_bool("enable_support");
    SupportType stype   = glb_cfg.opt_enum<SupportType>("support_type");
    bool on_buildplate_only = glb_cfg.opt_bool("support_on_build_plate_only");

    int cur_idx = 0;
    if (enable_support) {
        switch (stype) {
            // New order: 1 normal(auto), 2 normal(manual), 3 tree(auto), 4 tree(manual)
            case SupportType::stNormalAuto: cur_idx = on_buildplate_only ? 2 : 1; break;
            case SupportType::stTreeAuto:   cur_idx = on_buildplate_only ? 4 : 3; break;
            case SupportType::stNormal:     cur_idx = 2; break; // compatibility if manual types are present
            case SupportType::stTree:       cur_idx = 4; break; // compatibility if manual types are present
            default:                        cur_idx = 1; break;
        }
    }

    ImDrawList* dl   = ImGui::GetWindowDrawList();
    const bool  dark = wxGetApp().dark_mode();

    // Headings
    ImGui::PushStyleColor(ImGuiCol_Text, dark ? ImVec4(1,1,1,1) : ImVec4(0.17f,0.17f,0.18f,1));
    // Scale font to 16px for headings and restore after
    const float prev_win_scale = ImGui::GetFontSize() / ImGui::GetFont()->FontSize;
    // ImGui::SetWindowFontScale(18.0f / ImGui::GetFont()->FontSize);
    ImGui::SetWindowFontScale(1.18f);
    // ImGui::TextUnformatted(_u8L("Support").c_str());
    // ImGui::Dummy(ImVec2(0, 6.0f * scale));
    const std::string support_type_title = _u8L("Support type");
    const ImVec2 support_type_title_size = ImGui::CalcTextSize(support_type_title.c_str());
    ImGui::SetCursorPosX(std::max(0.0f, (ImGui::GetWindowWidth() - support_type_title_size.x) * 0.5f));
    ImGui::TextUnformatted(support_type_title.c_str());
    ImGui::SetWindowFontScale(prev_win_scale);
    ImGui::PopStyleColor();
    ImGui::Dummy(ImVec2(0, 6.0f * scale));

    // Five tiles with image + radio + text under each
    struct Tile { std::string label; const char* filename; };
    Tile tiles[5] = {
        { _u8L("No support"),                  "nosupport.png" },
        { _u8L("Normal"),                      "normal_auto.png" },
        { _u8L("Normal(On build plate only)"), "normal_onbuild.png" },
        { _u8L("Tree"),                        "tree_auto.png" },
        { _u8L("Tree(On build plate only)"),   "tree_onbuild.png" },
    };

    // Lazy-load ImGui textures from resources_dir()/images/
    static ImTextureID s_tex_id[5] = { };
    static bool s_loaded = false;
    if (!s_loaded) {
        const std::string base = Slic3r::resources_dir() + "/images/";
        unsigned w = (unsigned)std::round(138.0f * scale);
        unsigned h = w;
        for (int i = 0; i < 5; ++i) {
            ImTextureID id = nullptr;
            IMTexture::load_from_png_file(base + tiles[i].filename, w, h, id);
            s_tex_id[i] = id;
        }
        s_loaded = true;
    }

    const float  tile_gap    = 18.0f * scale;
    const float  left_inset  = 20.0f * scale;
    const float  right_inset = 20.0f * scale;
    const float  max_img     = 138.0f * scale;
    const float  min_img     = 48.0f * scale;
    const float  avail_w     = std::max(1.0f, ImGui::GetContentRegionAvail().x - left_inset - right_inset);
    const float  fit_img     = (avail_w - tile_gap * 4.0f) / 5.0f;
    const float  img_edge    = std::max(min_img, std::min(max_img, fit_img));
    const float  table_w     = img_edge * 5.0f + tile_gap * 4.0f;
    const ImVec2 img_sz(img_edge, img_edge);
    const ImU32  bg_col      = dark ? IM_COL32(28, 28, 30, 255) : IM_COL32(242, 242, 246, 255);
    const ImU32  br_grey     = dark ? IM_COL32(90, 90, 92, 255) : IM_COL32(180, 180, 185, 255);
    const ImU32  br_green    = IM_COL32(21, 192, 89, 255);
    const ImU32  br_hover    = IM_COL32(68, 205, 122, 255) ;
    const float  radio_r     = 7.0f * scale;
    const float  radio_th    = 1.7f * scale;
    const float  text_gap_y  = 6.0f * scale;
    const ImU32  radio_grey  = dark ? IM_COL32(140, 140, 145, 255) : IM_COL32(120, 120, 125, 255);
    const ImU32  radio_green = br_green;
    const ImU32  radio_fill  = IM_COL32(41, 190, 85, 180);
    // Text color follows dark/light mode (match general UI palette used elsewhere)
    const ImVec4 text_col    = dark ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f)
                                    : ImVec4(0.1686f, 0.1686f, 0.1765f, 1.0f);

    ImGui::SetCursorPosX(left_inset + std::max(0.0f, 0.5f * (avail_w - table_w)));
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(tile_gap * 0.5f, ImGui::GetStyle().CellPadding.y));
    // if (ImGui::BeginTable("##sup_tiles", 5, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoBordersInBody)) {
    if (ImGui::BeginTable("##sup_tiles", 5, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoBordersInBody | ImGuiTableFlags_NoPadOuterX)) {
        for (int i = 0; i < 5; ++i)
            ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthFixed, img_sz.x);

        for (int i = 0; i < 5; ++i) {
            ImGui::TableNextColumn();

            // Image tile background + image button
            ImVec2 tl = ImGui::GetCursorScreenPos();
            ImVec2 br = ImVec2(tl.x + img_sz.x, tl.y + img_sz.y);
            // Draw a subtle tile background to ensure transparent SVGs remain visible
            dl->AddRectFilled(tl, br, bg_col, 6.0f * scale);
            bool img_clicked = false;
            bool img_hovered = false;
            if (s_tex_id[i] != nullptr) {
                ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0,0,0,0));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0,0,0,0));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0,0,0,0));
                img_clicked = wxGetApp().imgui()->image_button(
                    s_tex_id[i], img_sz, ImVec2(0,0), ImVec2(1,1), 0,
                    ImVec4(0,0,0,0), ImVec4(1,1,1,1), 0);
                ImGui::PopStyleColor(3);
                img_hovered = ImGui::IsItemHovered();
            } else {
                ImGui::InvisibleButton((std::string("##sup_img_") + std::to_string(i)).c_str(), img_sz);
                img_clicked = ImGui::IsItemClicked();
                img_hovered = ImGui::IsItemHovered();
            }

            // Selection border around the image button area (slightly thicker for visibility)
            ImVec2 item_min = ImGui::GetItemRectMin();
            ImVec2 item_max = ImGui::GetItemRectMax();
            dl->AddRect(item_min, item_max, (i == cur_idx) ? br_green : br_grey, 6.0f * scale, 0, 2.0f * scale);

            // Radio + label row
            ImVec2 row_pos = ImGui::GetCursorScreenPos();
            row_pos.y += text_gap_y;
            ImVec2 center = ImVec2(row_pos.x + radio_r, row_pos.y + ImGui::GetTextLineHeightWithSpacing() * 0.5f);
            dl->AddCircle(center, radio_r, (i == cur_idx) ? radio_green : radio_grey, 0, radio_th);
            if (i == cur_idx)
                dl->AddCircleFilled(center, radio_r - 3.0f * scale, radio_fill);

            ImGui::SetCursorScreenPos(ImVec2(row_pos.x + radio_r * 2.0f + 8.0f * scale, row_pos.y));
            std::string display_label = tiles[i].label;
            const size_t paren_pos = display_label.find('(');
            if (paren_pos != std::string::npos)
                display_label.insert(paren_pos, "\n");
            const float label_wrap_x = row_pos.x + img_sz.x + 8.0f * scale;
            ImGui::PushStyleColor(ImGuiCol_Text, text_col);
            ImGui::PushTextWrapPos(label_wrap_x);
            ImGui::SetWindowFontScale(0.92f);
            // ImGui::TextUnformatted(tiles[i].label.c_str());
            ImGui::TextUnformatted(display_label.c_str());
            ImGui::SetWindowFontScale(1.0f);
            ImGui::PopTextWrapPos();
            ImGui::PopStyleColor(1);

            ImVec2 after    = ImGui::GetCursorScreenPos();
            // Make the clickable area under the image span the full tile width
            // ImVec2 label_sz = ImVec2(img_sz.x, ImGui::GetTextLineHeightWithSpacing());
            ImVec2 label_sz = ImVec2(img_sz.x, std::max(ImGui::GetTextLineHeightWithSpacing(), after.y - row_pos.y));
            ImGui::SetCursorScreenPos(row_pos);
            ImGui::InvisibleButton((std::string("##sup_row_") + std::to_string(i)).c_str(), label_sz);
            bool row_clicked = ImGui::IsItemClicked();
            bool row_hovered = ImGui::IsItemHovered();

            // Hover feedback: draw subtle border when hovering image or label row (if not selected)
            if ((img_hovered || row_hovered) && i != cur_idx) {
                dl->AddRect(tl, br, br_hover, 6.0f * scale, 0, 2.0f * scale);
            }

            if ((img_clicked || row_clicked) && i != cur_idx) {
                cur_idx = i;

                // Apply changes to the global edited print preset
                auto* preset_bundle = wxGetApp().preset_bundle;
                if (preset_bundle != nullptr) {
                    auto &cfg = preset_bundle->prints.get_edited_preset().config;
                    if (cur_idx == 0) {
                        cfg.set_key_value("enable_support", new Slic3r::ConfigOptionBool(false));
                        // Reset buildplate-only flag when disabling support
                        cfg.set_key_value("support_on_build_plate_only", new Slic3r::ConfigOptionBool(false));
                    } else {
                        cfg.set_key_value("enable_support", new Slic3r::ConfigOptionBool(true));

                        bool buildplate_only = false;
                        SupportType new_type = SupportType::stNormalAuto;
                        switch (cur_idx) {
                            case 1: new_type = SupportType::stNormalAuto; buildplate_only = false; break;
                            case 2: new_type = SupportType::stNormalAuto;   buildplate_only = true; break;
                            case 3: new_type = SupportType::stTreeAuto; buildplate_only = false;  break;
                            case 4: new_type = SupportType::stTreeAuto;   buildplate_only = true;  break;
                        }
                        cfg.set_key_value("support_type", new Slic3r::ConfigOptionEnum<SupportType>(new_type));
                        cfg.set_key_value("support_on_build_plate_only", new Slic3r::ConfigOptionBool(buildplate_only));
                    }

                    // Notify plater to propagate config changes
                    if (wxGetApp().plater() != nullptr)
                        wxGetApp().plater()->on_config_change(preset_bundle->full_config());
                }
            }
        }
        ImGui::EndTable();
    }
    ImGui::PopStyleVar();
}

}}} // namespace Slic3r::GUI::SupportSimple
