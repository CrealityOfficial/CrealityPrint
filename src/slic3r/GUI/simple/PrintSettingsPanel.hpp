#ifndef slic3r_PrintSettingsPanel_hpp_
#define slic3r_PrintSettingsPanel_hpp_

#include "imgui/imgui.h" // ImGui core
#include <string>

namespace Slic3r { 
namespace GUI {

class PrintSettingsPanel
{
public:
    struct PrintSettings
    {
        int   quality_index    = 1;  // 0: Fast, 1: Standard, 2: Fine
        int   infill_density   = 15; // Percentage: 0�C100
        int   top_layers       = 5;
        int   bottom_layers    = 4;
        int   wall_layers      = 4;
        float top_thickness    = 0.6f;
        float bottom_thickness = 0.5f;
        float wall_thickness   = 0.5f;
    };

    PrintSettingsPanel();
    ~PrintSettingsPanel();

    bool render();
    bool render(bool embedded);
    void on_popup();

    const PrintSettings& get_settings() const;
    void                 set_settings(const PrintSettings& s);

    void StyledInputFloat(const char* label, float* value, float width, ImU32 bg_color)
    {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, bg_color);
        ImGui::PushItemWidth(width);
        ImGui::InputFloat(label, value, 0.0f, 0.0f, "%.1f");
        ImGui::PopItemWidth();
        ImGui::PopStyleColor();
    }

    void StyledInputInt(const char* label, int* value, float width, ImU32 bg_color)
    {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, bg_color);
        ImGui::PushItemWidth(width);
        ImGui::InputInt(label, value, 0, 0);
        ImGui::PopItemWidth();
        ImGui::PopStyleColor();
    }

    bool StyledButton(const char* label, ImU32 bg_color, ImU32 hover_color, ImU32 active_color)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, bg_color);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hover_color);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, active_color);
        bool clicked = ImGui::Button(label);
        ImGui::PopStyleColor(3);
        return clicked;
    }

    bool StyledCombo(
        const char* label, int* current_index, const char* const items[], int item_count, float width, ImU32 bg_color, ImU32 button_color)
    {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, bg_color);
        ImGui::PushStyleColor(ImGuiCol_Button, button_color); // ���Ƽ�ͷ��ť��ɫ
        ImGui::PushStyleColor(ImGuiCol_PopupBg, bg_color);    // �����˵�����
        ImGui::PushItemWidth(width);

        bool changed = ImGui::Combo(label, current_index, items, item_count);

        ImGui::PopItemWidth();
        ImGui::PopStyleColor(3);

        return changed;
    }

private:
    static constexpr const char* infill_options[] = {"5%", "10%", "15%", "20%", "25%", "30%", "40%", "50%"};
    
    // �ٷֱ� �� ����
    int infill_density_to_index(int density)
    {
        for (int i = 0; i < IM_ARRAYSIZE(infill_options); ++i) {
            if (std::stoi(infill_options[i]) == density)
                return i;
        }
        return 0; // Ĭ�ϻ��˵���һ��
    }

    // ���� �� �ٷֱ�
    int infill_index_to_density(int index)
    {
        if (index >= 0 && index < IM_ARRAYSIZE(infill_options))
            return std::stoi(infill_options[index]);
        return 15; // Ĭ��ֵ
    }

    template<typename T> void safe_get(const DynamicPrintConfig& cfg, const char* key, T& out)
    {
        if (auto* opt = cfg.option<typename ConfigOptionType<T>::type>(key))
            out = opt->value;
    }
    template<typename T> void safe_set(DynamicPrintConfig& cfg, const char* key, const T& val)
    {
        if (auto* opt = cfg.option<typename ConfigOptionType<T>::type>(key))
            opt->value = val;
    }
    // Primary template only; explicit specializations live at namespace scope
    // below (GCC rejects explicit specialization inside a class body, unlike MSVC).
    template<typename T> struct ConfigOptionType;

    void load_from_config(PrintSettings& s, const DynamicPrintConfig& config)
    {
        // ������λ��������
     /*   if (auto* opt = config.option<ConfigOptionFloat>("sparse_infill_pattern")) {
            float lh = opt->value;
            if (lh == 0.20f)
                s.quality_index = 0;
            else if (lh == 0.16f)
                s.quality_index = 1;
            else
                s.quality_index = 2;
        }*/

        safe_get(config, "top_shell_thickness", s.top_thickness);
        safe_get(config, "bottom_shell_thickness", s.bottom_thickness);
        safe_get(config, "top_shell_layers", s.top_layers);
        safe_get(config, "bottom_shell_layers", s.bottom_layers);
        safe_get(config, "wall_thickness", s.wall_thickness);
        safe_get(config, "perimeters", s.wall_layers);
        unsigned tmp_density = 0;
        safe_get(config, "sparse_infill_density", tmp_density);
        s.infill_density = static_cast<int>(tmp_density);
    }

    void apply_to_config(const PrintSettings& s, DynamicPrintConfig& config)
    {
       // float lh = (s.quality_index == 0) ? 0.20f : (s.quality_index == 1) ? 0.16f : 0.12f;
      //  safe_set(config, "sparse_infill_pattern", lh);

        safe_set(config, "top_shell_thickness", s.top_thickness);
        safe_set(config, "bottom_shell_thickness", s.bottom_thickness);
        safe_set(config, "top_shell_layers", s.top_layers);
        safe_set(config, "bottom_shell_layers", s.bottom_layers);
        safe_set(config, "wall_thickness", s.wall_thickness);
        safe_set(config, "perimeters", s.wall_layers);
        safe_set(config, "sparse_infill_density", static_cast<unsigned>(s.infill_density));
    }



    PrintSettings settings;
    float         animated_highlight_x = -1.0f; // ��ʼ��Ϊ��Чֵ
    bool          m_initialized = false;
};

// Explicit specializations of the member trait, required at namespace scope by GCC.
template<> struct PrintSettingsPanel::ConfigOptionType<int>      { using type = ConfigOptionInt; };
template<> struct PrintSettingsPanel::ConfigOptionType<float>    { using type = ConfigOptionFloat; };
template<> struct PrintSettingsPanel::ConfigOptionType<unsigned> { using type = ConfigOptionPercent; };

using PrintSettings = PrintSettingsPanel::PrintSettings;
}
} // namespace slic3r

#endif // slic3r_PrintSettingsPanel_hpp_
