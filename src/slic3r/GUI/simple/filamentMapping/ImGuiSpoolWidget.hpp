#pragma once

#include "imgui/imgui.h"

namespace Slic3r {
namespace GUI {

struct ImGuiSpoolVisualSpec {
    ImVec4 filament_color = ImVec4(1.f, 1.f, 1.f, 1.f);
    float  alpha = 1.f;
};

void draw_spool_widget(ImDrawList* draw_list, const ImVec2& pos, const ImVec2& size, const ImGuiSpoolVisualSpec& spec);

} // namespace GUI
} // namespace Slic3r
