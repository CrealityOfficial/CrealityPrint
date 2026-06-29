#include "PrintSettingsPanel.hpp"
#include <string>
#include <array>
#include <regex>
#include <limits>
#include <cmath>
#include "GUI_App.hpp"
#include "slic3r/GUI/Tab.hpp"
#include "ImGuiWrapper.hpp"
#include "slic3r/GUI/I18N.hpp"
#include "slic3r/GUI/PresetComboBoxes.hpp"
#include "Field.hpp"

namespace Slic3r { namespace GUI {

PrintSettingsPanel::PrintSettingsPanel() { settings = PrintSettings{}; }

PrintSettingsPanel::~PrintSettingsPanel()
{
}

const PrintSettingsPanel::PrintSettings& PrintSettingsPanel::get_settings() const { return settings; }

void PrintSettingsPanel::set_settings(const PrintSettings& s) { settings = s; }

bool PrintSettingsPanel::render()
{ return render(false); }

// 头文件里把签名改一下：bool render(bool embedded = false);

bool PrintSettingsPanel::render(bool embedded)
{
    bool settings_changed = false;

    auto& config = GUI::wxGetApp().preset_bundle->prints.get_edited_preset().config;
    if (!m_initialized) {
        load_from_config(settings, config);
        m_initialized = true;
    }

    const bool is_dark = Slic3r::GUI::wxGetApp().dark_mode();
    // 集中颜色
    const ImU32 slider_bg_color      = is_dark ? IM_COL32(60, 60, 60, 255)  : IM_COL32(230, 230, 230, 255);
    const ImU32 highlight_color      = is_dark ? IM_COL32(200, 200, 200, 255): IM_COL32(255, 255, 255, 255);
    const ImU32 highlight_text_color = IM_COL32(21, 192, 89, 255);
    const ImU32 slider_text_color    = IM_COL32(160, 160, 160, 255);
    const ImU32 label_text_color     = is_dark ? IM_COL32(255, 255, 255, 255): IM_COL32(30, 30, 30, 255);
    const ImU32 panel_bg_color       = is_dark ? IM_COL32(30, 30, 30, 255)   : IM_COL32(255, 255, 255, 255);

    // 嵌入模式：外层卡片窗口已经绘制了白色圆角背景，这里只设置文字颜色；独立模式才设置 WindowBg
    if (!embedded) {
        ImGui::PushStyleColor(ImGuiCol_WindowBg, panel_bg_color);
        ImGui::PushStyleColor(ImGuiCol_Text,     label_text_color);
        ImGui::Begin("Print Settings", nullptr,
            ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar);
    } else {
        ImGui::PushStyleColor(ImGuiCol_Text,     label_text_color);
        // 不要 Push WindowBg，避免覆盖外层卡片
    }


    ImGui::Indent(20.0f); // 所有控件向右缩进 20 像素


    // 标题（可直接放大字号；SetWindowFontScale 对当前窗口有效，嵌入时同样生效）
    ImGui::SetWindowFontScale(1.5f);
    ImGui::TextUnformatted(_u8L("Print Settings").c_str());
    ImGui::SetWindowFontScale(1.0f);

    // ====== 控件区域（与原来一致；注意不要 ImGui::End()）======

    // --- Print Quality Slider ---
    struct QualityOption {
        int         preset_index = -1;
        std::string label;
        float       layer_height = 0.0f;
    };

    auto trim_quality_label = [](std::string s)->std::string {
        // keep leading layer height and the first word after it; drop the rest
        std::regex re(R"((\d+(?:\.\d+)?)mm\s+([A-Za-z]+))");
        std::smatch m;
        if (std::regex_search(s, m, re) && m.size() >= 3) {
            return m[1].str() + " " + m[2].str();
        }
        return s;
    };

    auto extract_layer_height = [](const std::string& s)->float {
        std::regex re(R"(([0-9]+(?:\.[0-9]+)?)mm)");
        std::smatch m;
        if (std::regex_search(s, m, re)) {
            try { return std::stof(m[1].str()); } catch (...) {}
        }
        return 0.0f;
    };

    auto build_quality_options = [&](TabPresetComboBox* combo)->std::array<QualityOption,3> {
        std::array<QualityOption,3> defaults {{
            { -1, "Fast",     0.12f },
            { -1, "Standard", 0.20f },
            { -1, "Fine",     0.28f }
        }};
        if (!combo) return defaults;

        const unsigned count = combo->GetCount();
        if (count == 0) return defaults;

        std::vector<QualityOption> all;
        all.reserve(count);
        for (unsigned i = 0; i < count; ++i) {
            std::string label = combo->GetString(i).ToUTF8().data();
            all.push_back({ static_cast<int>(i), trim_quality_label(label), extract_layer_height(label) });
        }

        const float target = 0.20f;
        int mid_idx = -1;
        float best = std::numeric_limits<float>::max();
        for (size_t i = 0; i < all.size(); ++i) {
            float d = std::fabs(all[i].layer_height - target);
            if ((all[i].layer_height > 0.0f) && d < best) {
                best = d; mid_idx = static_cast<int>(i);
            }
            if (all[i].layer_height > 0.0f && all[i].layer_height == target) {
                if (all[i].label.find("Standard") != std::string::npos) {
                    mid_idx = static_cast<int>(i);
                    break;
                }
            }
        }
        if (mid_idx < 0) mid_idx = 0;

        int left_idx = mid_idx;
        float left_height = -1.0f;
        for (int i = mid_idx - 1; i >= 0; --i) {
            if (all[i].layer_height > 0.0f && all[i].layer_height < all[mid_idx].layer_height) {
                left_idx = i; left_height = all[i].layer_height; break;
            }
        }
        if (left_height < 0.0f && mid_idx > 0) left_idx = mid_idx - 1;

        int right_idx = mid_idx;
        float right_height = -1.0f;
        for (size_t i = mid_idx + 1; i < all.size(); ++i) {
            if (all[i].layer_height > 0.0f && all[i].layer_height > all[mid_idx].layer_height) {
                right_idx = static_cast<int>(i); right_height = all[i].layer_height; break;
            }
        }
        if (right_height < 0.0f && mid_idx + 1 < (int)all.size()) right_idx = mid_idx + 1;

        std::array<QualityOption,3> out;
        out[0] = all[left_idx];
        out[1] = all[mid_idx];
        out[2] = all[right_idx];
        return out;
    };

    TabPresetComboBox* process_preset_combo = nullptr;
    {
        Tab* tab_print = wxGetApp().get_tab(Preset::TYPE_PRINT);
        if (tab_print) process_preset_combo = tab_print->get_combo_box();
    }

    auto quality_options = build_quality_options(process_preset_combo);
    const char* quality_labels[] = {
        quality_options[0].label.c_str(),
        quality_options[1].label.c_str(),
        quality_options[2].label.c_str()
    };
    const int quality_to_preset_indices[] = {
        quality_options[0].preset_index,
        quality_options[1].preset_index,
        quality_options[2].preset_index
    };
    const int   num_options      = 3;
    const float item_width       = 100.0f;
    const float item_spacing     = 40.0f;
    const float edge_padding     = 40.0f;
    const float control_height   = 40.0f;
    const float control_width    = edge_padding * 2 + num_options * item_width + (num_options - 1) * item_spacing;

    ImVec2      cursor_pos = ImGui::GetCursorScreenPos();
    ImDrawList* draw_list  = ImGui::GetWindowDrawList();

    const float label_offset_y = (control_height - ImGui::GetFontSize()) / 2;

    // Label
    ImGui::SetCursorScreenPos(ImVec2(cursor_pos.x, cursor_pos.y + label_offset_y));
    ImGui::TextUnformatted(_u8L("Print Quality").c_str());
    ImGui::SameLine();

    // Slider background
    cursor_pos = ImGui::GetCursorScreenPos();
    const int right_offset = 30;
    ImGui::SetCursorScreenPos(ImVec2(cursor_pos.x + right_offset, cursor_pos.y - label_offset_y));
    ImVec2 slider_start  = ImGui::GetCursorScreenPos();
    ImVec2 bg_min        = slider_start;
    ImVec2 bg_max        = ImVec2(slider_start.x + control_width, slider_start.y + control_height);
    float  corner_radius = control_height / 2.0f;
    draw_list->AddRectFilled(bg_min, bg_max, slider_bg_color, corner_radius);

    // Highlight block
    if (settings.quality_index < 0 || settings.quality_index >= num_options)
        settings.quality_index = 1;

    float highlight_width    = item_width + item_spacing;
    float target_highlight_x = slider_start.x + edge_padding + (item_width + item_spacing) * settings.quality_index - item_spacing / 2.0f;

    if (animated_highlight_x < 0.0f) {
        animated_highlight_x = target_highlight_x;
    } else {
        float delta = target_highlight_x - animated_highlight_x;
        if (fabsf(delta) <= 0.5f) animated_highlight_x = target_highlight_x;
        else                      animated_highlight_x += delta * 0.8f;
    }

    if (settings.quality_index < 0 || settings.quality_index >= num_options)
        settings.quality_index = 1;

    float highlight_right = animated_highlight_x + highlight_width;
    if (highlight_right > bg_max.x) highlight_right = bg_max.x;

    const float highlight_padding_y = 5.0f;
    ImVec2 highlight_min = ImVec2(animated_highlight_x, slider_start.y + highlight_padding_y);
    ImVec2 highlight_max = ImVec2(highlight_right, slider_start.y + control_height - highlight_padding_y);

    draw_list->AddRectFilled(highlight_min, highlight_max, highlight_color, corner_radius);

    // Labels + 点击
    for (int i = 0; i < num_options; ++i) {
        float item_x  = slider_start.x + edge_padding + (item_width + item_spacing) * i;
        float label_x = item_x + item_width / 2;
        float label_y = slider_start.y + control_height / 2;

        ImVec2 text_size  = ImGui::CalcTextSize(_u8L(quality_labels[i]).c_str());
        ImVec2 text_pos   = ImVec2(label_x - text_size.x / 2, label_y - text_size.y / 2);
        ImU32  text_color = (i == settings.quality_index) ? highlight_text_color : slider_text_color;
        draw_list->AddText(text_pos, text_color, _u8L(quality_labels[i]).c_str());

        ImGui::SetCursorScreenPos(ImVec2(item_x, slider_start.y));
        if (ImGui::InvisibleButton(("##QualityBtn" + std::to_string(i)).c_str(),
                                   ImVec2(item_width + item_spacing, control_height))) {

            const int preset_idx = quality_to_preset_indices[i];
            if (process_preset_combo && preset_idx >= 0 && preset_idx < (int)process_preset_combo->GetCount()) {
                process_preset_combo->SetSelection(preset_idx);
                wxCommandEvent evt(wxEVT_COMBOBOX, process_preset_combo->GetId());
                evt.SetInt(preset_idx);
                evt.SetEventObject(process_preset_combo);
                process_preset_combo->OnSelect(evt);
            }

            settings.quality_index = i;
        }
    }

    ImGui::Dummy(ImVec2(control_width, 10));

    // 其他控件（BeginTable 在嵌入/独立两种模式下都可以用）

    const ImU32 bg_color    = is_dark ? IM_COL32(60, 60, 60, 255)  : IM_COL32(230, 230, 230, 255);
    const ImU32 hover_color = is_dark ? IM_COL32(80, 80, 80, 255)  : IM_COL32(200, 200, 200, 255);
    const ImU32 active_color= is_dark ? IM_COL32(100,100,100,255)  : IM_COL32(180,180,180,255);

    if (ImGui::BeginTable("PrintSettingsTable", 4, ImGuiTableFlags_SizingFixedFit)) { // ImGuiTableFlags_SizingStretchProp

        const int image_width  = 200;
        const int image_height = 100;

        ImGui::TableSetupColumn("Col0", ImGuiTableColumnFlags_WidthFixed, image_width+10.0f);
        ImGui::TableSetupColumn("Col1", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Col2", ImGuiTableColumnFlags_WidthFixed, 160.0f);
        ImGui::TableSetupColumn("Col3", ImGuiTableColumnFlags_WidthFixed, 160.0f);

        // 设置统一的垂直间距
        const int vertical_spacing = 20;

        // 第一行：标签 + Reset
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted(_u8L("Print fill").c_str());
        ImGui::TableSetColumnIndex(1); ImGui::TextUnformatted(_u8L("Print Strength").c_str());
        ImGui::TableSetColumnIndex(2);
        {
            // Reset 图标+按钮（建议缓存纹理，不要每帧 load）
            ImTextureID texture_id = nullptr;
            static ImTextureID s_reset_tex = nullptr;
            if (!s_reset_tex) {
                IMTexture::load_from_svg_file(Slic3r::resources_dir() + "/images/toolbar_reset.svg", 16, 16, s_reset_tex);
            }
            if (s_reset_tex) ImGui::Image(s_reset_tex, ImVec2(16,16));
            ImGui::SameLine();
            if (StyledButton(_u8L("Reset").c_str(), bg_color, hover_color, active_color)) {
                settings         = PrintSettings{};
                settings_changed = true;
            }
        }


        // 第二行：图片 + Top/Bottom Shell 内容（合并为一行，高度足够）
        ImGui::TableNextRow(); // 一行，高度够两组控件

        // 第0列：图片
        ImGui::TableSetColumnIndex(0);
        {
            static ImTextureID s_sparse_tex = nullptr;
            if (!s_sparse_tex) {
                IMTexture::load_from_svg_file(Slic3r::resources_dir() + "/images/sparse_infill_density.svg", image_width, image_height,
                                              s_sparse_tex);
            }
            if (s_sparse_tex)
                ImGui::Image(s_sparse_tex, ImVec2(image_width, image_height));
        }

        // 第1列：Top Shell + Bottom Shell 标签
        ImGui::TableSetColumnIndex(1);
        ImGui::Dummy(ImVec2(0, vertical_spacing)); // 图片底部 → Top Shell
        ImGui::TextUnformatted(_u8L("Top Shell").c_str());
        ImGui::Dummy(ImVec2(0, vertical_spacing)); // Top Shell → Bottom Shell
        ImGui::TextUnformatted(_u8L("Bottom Shell").c_str());

        // 第2列：TopThickness + BottomThickness
        ImGui::TableSetColumnIndex(2);
        ImGui::Dummy(ImVec2(0, vertical_spacing));
        StyledInputFloat("##TopThickness", &settings.top_thickness, 100, slider_bg_color);
        ImGui::SameLine();
        ImGui::TextUnformatted(_u8L("mm").c_str());
        ImGui::Dummy(ImVec2(0, vertical_spacing));
        StyledInputFloat("##BottomThickness", &settings.bottom_thickness, 100, slider_bg_color);
        ImGui::SameLine();
        ImGui::TextUnformatted(_u8L("mm").c_str());

        // 第3列：TopLayers + BottomLayers
        ImGui::TableSetColumnIndex(3);
        ImGui::Dummy(ImVec2(0, vertical_spacing));
        StyledInputInt("##TopLayers", &settings.top_layers, 100, slider_bg_color);
        ImGui::SameLine();
        ImGui::TextUnformatted(_u8L("layers").c_str());
        ImGui::Dummy(ImVec2(0, vertical_spacing));
        StyledInputInt("##BottomLayers", &settings.bottom_layers, 100, slider_bg_color);
        ImGui::SameLine();
        ImGui::TextUnformatted(_u8L("layers").c_str());

        // 可选：Bottom Shell 往下的间距
        ImGui::Dummy(ImVec2(0, vertical_spacing));



        // 第四行：Infill + Wall + Perimeters
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted(_u8L("Infill Density").c_str());
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        int infill_index = infill_density_to_index(settings.infill_density);

        if (StyledCombo("##InfillCombo", &infill_index, infill_options, IM_ARRAYSIZE(infill_options), 100, bg_color, hover_color)) {
            settings.infill_density = infill_index_to_density(infill_index);
            //settings_changed        = true;
            safe_set(config, "sparse_infill_density", static_cast<unsigned>(settings.infill_density));
        }
        ImGui::TableSetColumnIndex(1); ImGui::TextUnformatted(_u8L("Wall").c_str());
        ImGui::TableSetColumnIndex(2);
        StyledInputFloat("##WallThickness", &settings.wall_thickness, 100, slider_bg_color);
        ImGui::SameLine();
        ImGui::TextUnformatted(_u8L("mm").c_str());
        ImGui::TableSetColumnIndex(3);
        StyledInputInt("##PerimeterCount", &settings.wall_layers, 100, slider_bg_color);
        ImGui::SameLine();
        ImGui::TextUnformatted(_u8L("perimeters").c_str());

        ImGui::EndTable();
        // ImGui::Dummy(ImVec2(0, 20));
    }

    ImGui::Unindent(20.0f);

    if (settings_changed) {
        apply_to_config(settings, config);
        load_from_config(settings, config);
    }

    // ====== 收尾 ======
    if (!embedded) {
        ImGui::End();
        ImGui::PopStyleColor(2);
    } else {
        ImGui::PopStyleColor(1);
    }

    return settings_changed;
}

void PrintSettingsPanel::on_popup()
{
    auto& config = GUI::wxGetApp().preset_bundle->prints.get_edited_preset().config;
    load_from_config(settings, config);
}

} // namespace GUI
} // namespace slic3r
