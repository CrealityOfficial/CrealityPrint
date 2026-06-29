// Minimal ImGui mirror for the wx-based FilamentPanel
// This file is a starting point to migrate UI logic from wxWidgets to ImGui.
// It reads/writes the same Slic3r config to ensure parity of side-effects.

#include "ImGuiFilamentPanel.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

// Pull required Slic3r headers to talk to project_config and plater.
#include "../../GUI_App.hpp"
#include "../../Plater.hpp"
#include "../../GLCanvas3D.hpp"
#include "libslic3r/Config.hpp"
#include "libslic3r/Preset.hpp"
#include "../../Tab.hpp"
#include "../../print_manage/data/DataCenter.hpp"
#include "../../PartPlate.hpp"

namespace Slic3r {
namespace GUI {

/*
void ImGuiFilamentPanel::render_palette_view()
{
    const float scale = GUI::wxGetApp().plater()->get_current_canvas3D()->get_scale();

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.16f, 0.2f, 0.26f, 1.0f));
    ImGui::TextUnformatted("MATERIAL PALETTE");
    ImGui::PopStyleColor(1);

    ImGui::Dummy(ImVec2(0.0f, 12.f * scale));

    ImVec2 block_size(152.f * scale, 52.f * scale);
    const float gap = 8.f * scale;
    ImDrawList* dl = ImGui::GetWindowDrawList();

    ImVec2 row_start = ImGui::GetCursorScreenPos();
    ImVec4 color = ImVec4(0.98f, 0.55f, 0.26f, 1.0f); // Ĭ�ϳ�ɫ
    std::string color_label = "Orange";
    if (!m_items.empty()) {
        color = m_items.front().color;
        color_label = m_items.front().type_label.empty() ? (m_items.front().preset_display.empty() ? "Color" : m_items.front().preset_display) : m_items.front().type_label;
    }

    // �����ɫ��
    ImVec2 color_min = row_start;
    ImVec2 color_max = ImVec2(row_start.x + block_size.x, row_start.y + block_size.y);
    dl->AddRectFilled(color_min, color_max, ImGui::GetColorU32(color), 4.f);
    dl->AddRect(color_min, color_max, ImGui::GetColorU32(ImVec4(0.85f, 0.85f, 0.85f, 1.0f)), 4.f, 0, 1.2f);
    ImVec2 color_txt_sz = ImGui::CalcTextSize(color_label.c_str());
    dl->AddText(ImVec2(color_min.x + (block_size.x - color_txt_sz.x) * 0.5f,
                       color_min.y + (block_size.y - color_txt_sz.y) * 0.5f),
                ImGui::GetColorU32(is_dark_text_on(color) ? ImVec4(0,0,0,1) : ImVec4(1,1,1,1)),
                color_label.c_str());

    // ��ɫռλ "Select a material"
    ImVec2 sel_min = ImVec2(color_max.x + gap, row_start.y);
    ImVec2 sel_max = ImVec2(sel_min.x + block_size.x, sel_min.y + block_size.y);
    ImVec4 sel_bg(0.94f, 0.94f, 0.94f, 1.0f);
    ImVec4 sel_txt(0.6f, 0.6f, 0.6f, 1.0f);
    dl->AddRectFilled(sel_min, sel_max, ImGui::GetColorU32(sel_bg), 4.f);
    dl->AddRect(sel_min, sel_max, ImGui::GetColorU32(ImVec4(0.85f, 0.85f, 0.85f, 1.0f)), 4.f, 0, 1.0f);
    const char* sel_label = "Select a material";
    ImVec2 sel_txt_sz = ImGui::CalcTextSize(sel_label);
    dl->AddText(ImVec2(sel_min.x + (block_size.x - sel_txt_sz.x) * 0.5f,
                       sel_min.y + (block_size.y - sel_txt_sz.y) * 0.5f),
                ImGui::GetColorU32(sel_txt),
                sel_label);

    ImGui::Dummy(ImVec2(block_size.x * 2 + gap, block_size.y));
    ImGui::Dummy(ImVec2(0.0f, 18.f * scale));

    // �в� +Add
    float add_w = 170.f * scale;
    float add_h = 34.f * scale;
    float avail_w = ImGui::GetContentRegionAvail().x;
    float add_x = ImGui::GetCursorPosX() + std::max(0.0f, (avail_w - add_w) * 0.5f);
    ImGui::SetCursorPosX(add_x);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
    if (ImGui::Button("+ Add", ImVec2(add_w, add_h))) {
        // Ԥ����ⲿ�ɰ���Ӳ�λ
    }
    ImGui::PopStyleVar(1);
}
*/

} // namespace GUI
} // namespace Slic3r

