#include "FlushVolume.hpp"
#include "../GUI/GUI.hpp"
#include "../GUI/GUI_App.hpp"
#include "../GUI/Plater.hpp"
#include "libslic3r/FlushVolCalc.hpp"

#include <boost/log/trivial.hpp>

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
    if (project_config.option<ConfigOptionBool>("flush_volumes_changed")->value)
        return;
        
    PresetBundle& preset_bundle = *wxGetApp().preset_bundle;
    std::vector<double> m_matrix = project_config.option<ConfigOptionFloats>("flush_volumes_matrix")->values;
    const std::vector<std::string> extruder_colours = wxGetApp().plater()->get_extruder_colors_from_plater_config();
    const auto full_config = preset_bundle.full_config();
    const size_t filament_count = preset_bundle.filament_presets.size();
    const size_t nozzle_count = size_t(std::max(1, preset_bundle.get_printer_extruder_count()));
    const size_t block_size = filament_count * filament_count;
    const size_t expected_size = block_size * nozzle_count;
    const bool standard_matrix = m_matrix.size() == expected_size;
    const bool legacy_matrix = nozzle_count > 1 && m_matrix.size() == block_size;

    if (filament_count == 0 || (!standard_matrix && !legacy_matrix) || extruder_colours.size() < filament_count) {
        BOOST_LOG_TRIVIAL(error) << "[FlushVolume] Invalid flushing data: matrix.size=" << m_matrix.size()
                                   << ", expected_size=" << expected_size
                                   << ", legacy_size=" << block_size
                                   << ", filament_count=" << filament_count
                                   << ", nozzle_count=" << nozzle_count
                                   << ", colours.size=" << extruder_colours.size();
        return;
    }

    if (legacy_matrix) {
        std::vector<double> expanded_matrix;
        expanded_matrix.reserve(expected_size);
        for (size_t nozzle_id = 0; nozzle_id < nozzle_count; ++nozzle_id)
            expanded_matrix.insert(expanded_matrix.end(), m_matrix.begin(), m_matrix.end());
        m_matrix = std::move(expanded_matrix);
    }

    std::vector<wxColour> m_colours;
    for (size_t index = 0; index < filament_count; ++index) {
        Slic3r::ColorRGB rgb;
        Slic3r::decode_color(extruder_colours[index], rgb);
        m_colours.push_back(wxColor(rgb.r_uchar(), rgb.g_uchar(), rgb.b_uchar()));
    }

    auto&  ams_multi_color_filament = wxGetApp().preset_bundle->ams_multi_color_filment;
    std::vector<std::vector<wxColour>> multi_colors;

    // Support for multi-color filament
    for (size_t i = 0; i < m_colours.size(); ++i) {
        std::vector<wxColour> single_filament;
        if (i < ams_multi_color_filament.size()) {
            if (!ams_multi_color_filament[i].empty()) {
                std::vector<std::string> colors = ams_multi_color_filament[i];
                for (size_t j = 0; j < colors.size(); ++j) {
                    single_filament.push_back(wxColour(colors[j]));
                }
                multi_colors.push_back(single_filament);
                continue;
            }
        }
        single_filament.push_back(wxColour(m_colours[i]));
        multi_colors.push_back(single_filament);
    }

    for (size_t nozzle_id = 0; nozzle_id < nozzle_count; ++nozzle_id) {
        const std::vector<int> min_flush_volume = get_min_flush_volumes(full_config, nozzle_id);
        if (min_flush_volume.size() < filament_count) {
            BOOST_LOG_TRIVIAL(error) << "[FlushVolume] Invalid minimum flushing volume data: nozzle_id=" << nozzle_id
                                       << ", size=" << min_flush_volume.size()
                                       << ", filament_count=" << filament_count;
            return;
        }

        const size_t matrix_offset = nozzle_id * block_size;
        for (size_t from_idx = 0; from_idx < multi_colors.size(); ++from_idx) {
            const bool is_from_support = is_support_filament(int(from_idx));
            for (size_t to_idx = 0; to_idx < multi_colors.size(); ++to_idx) {
                const size_t matrix_index = matrix_offset + filament_count * from_idx + to_idx;
                if (from_idx == to_idx) {
                    m_matrix[matrix_index] = 0;
                    continue;
                }

                const bool is_to_support = is_support_filament(int(to_idx));
                int flushing_volume = 0;
                if (is_to_support) {
                    flushing_volume = Slic3r::g_flush_volume_to_support;
                } else {
                    for (size_t i = 0; i < multi_colors[from_idx].size(); ++i) {
                        const wxColour& from = multi_colors[from_idx][i];
                        for (size_t j = 0; j < multi_colors[to_idx].size(); ++j) {
                            const wxColour& to     = multi_colors[to_idx][j];
                            int             volume = calc_flushing_volume(from, to, min_flush_volume[from_idx]);
                            flushing_volume        = std::max(flushing_volume, volume);
                        }
                    }

                    if (is_from_support) {
                        flushing_volume = std::max(Slic3r::g_min_flush_volume_from_support, flushing_volume);
                    }
                }
                m_matrix[matrix_index] = flushing_volume;
            }
        }
    }
    project_config.option<ConfigOptionFloats>("flush_volumes_matrix")->values = m_matrix;
}

} // namespace Utils
} // namespace Slic3r
