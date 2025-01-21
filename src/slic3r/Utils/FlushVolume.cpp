#include "FlushVolume.hpp"
#include "../GUI/GUI.hpp"
#include "../GUI/GUI_App.hpp"
#include "../GUI/Plater.hpp"

using namespace Slic3r::GUI;

namespace Slic3r {
namespace Utils {

int calc_flushing_volume(const wxColour& from_, const wxColour& to_, int min_flush_volume)
{
    Slic3r::FlushVolCalculator calculator(min_flush_volume, Slic3r::g_max_flush_volume);

    return calculator.calc_flush_vol(from_.Alpha(), from_.Red(), from_.Green(), from_.Blue(), to_.Alpha(), to_.Red(), to_.Green(),
                                     to_.Blue());
}

void calc_flushing_volumes()
{
    auto&                      project_config   = wxGetApp().preset_bundle->project_config;
    //
    std::vector<double> m_matrix  = (project_config.option<ConfigOptionFloats>("flush_volumes_matrix"))->values;
    const std::vector<double>& init_extruders   = (project_config.option<ConfigOptionFloats>("flush_volumes_vector"))->values;
    ConfigOptionFloat*         flush_multi_opt  = project_config.option<ConfigOptionFloat>("flush_multiplier");
    float                      flush_multiplier = flush_multi_opt ? flush_multi_opt->getFloat() : 1.f;

    const std::vector<std::string> extruder_colours    = wxGetApp().plater()->get_extruder_colors_from_plater_config();
    const auto&                    full_config         = wxGetApp().preset_bundle->full_config();
    std::vector<int> m_min_flush_volume  = get_min_flush_volumes(full_config);
    unsigned int m_number_of_extruders                 = (int) (sqrt(m_matrix.size()) + 0.001);

    std::vector<wxColour> m_colours;
    for (const std::string& color : extruder_colours) {
        Slic3r::ColorRGB rgb;
        Slic3r::decode_color(color, rgb);
        m_colours.push_back(wxColor(rgb.r_uchar(), rgb.g_uchar(), rgb.b_uchar()));
    }

    auto&  ams_multi_color_filament = wxGetApp().preset_bundle->ams_multi_color_filment;
    std::vector<std::vector<wxColour>> multi_colors;

    // Support for multi-color filament
    for (int i = 0; i < m_colours.size(); ++i) {
        std::vector<wxColour> single_filament;
        if (i < ams_multi_color_filament.size()) {
            if (!ams_multi_color_filament[i].empty()) {
                std::vector<std::string> colors = ams_multi_color_filament[i];
                for (int j = 0; j < colors.size(); ++j) {
                    single_filament.push_back(wxColour(colors[j]));
                }
                multi_colors.push_back(single_filament);
                continue;
            }
        }
        single_filament.push_back(wxColour(m_colours[i]));
        multi_colors.push_back(single_filament);
    }

    for (int from_idx = 0; from_idx < multi_colors.size(); ++from_idx) {
        bool is_from_support = is_support_filament(from_idx);
        for (int to_idx = 0; to_idx < multi_colors.size(); ++to_idx) {
            bool is_to_support = is_support_filament(to_idx);
            if (from_idx == to_idx) {
                ;// edit_boxes[to_idx][from_idx]->SetValue(std::to_string(0));
            } else {
                int flushing_volume = 0;
                if (is_to_support) {
                    flushing_volume = Slic3r::g_flush_volume_to_support;
                } else {
                    for (int i = 0; i < multi_colors[from_idx].size(); ++i) {
                        const wxColour& from = multi_colors[from_idx][i];
                        for (int j = 0; j < multi_colors[to_idx].size(); ++j) {
                            const wxColour& to     = multi_colors[to_idx][j];
                            int             volume = calc_flushing_volume(from, to, m_min_flush_volume[from_idx]);
                            flushing_volume        = std::max(flushing_volume, volume);
                        }
                    }

                    if (is_from_support) {
                        flushing_volume = std::max(Slic3r::g_min_flush_volume_from_support, flushing_volume);
                    }
                }

                m_matrix[m_number_of_extruders * from_idx + to_idx] = flushing_volume;
                //flushing_volume                                     = int(flushing_volume * get_flush_multiplier());
                //edit_boxes[to_idx][from_idx]->SetValue(std::to_string(flushing_volume));
            }
        }
    }
    project_config.option<ConfigOptionFloats>("flush_volumes_matrix")->values = m_matrix;
}

} // namespace Utils
} // namespace Slic3r
