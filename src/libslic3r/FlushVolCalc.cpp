#include "FlushVolCalc.hpp"
#include <cmath>
#include <assert.h>
#include "slic3r/Utils/ColorSpaceConvert.hpp"
#include "libslic3r/Color.hpp"

namespace Slic3r {

const int g_min_flush_volume_from_support = 700.f;
const int g_flush_volume_to_support = 230;

const int g_max_flush_volume = 1200;

static float to_radians(float degree)
{
    return degree / 180.f * M_PI;
}


static float get_luminance(float r, float g, float b)
{
    return r * 0.3 + g * 0.59 + b * 0.11;
}

static float calc_triangle_3rd_edge(float edge_a, float edge_b, float degree_ab)
{
    return std::sqrt(edge_a * edge_a + edge_b * edge_b - 2 * edge_a * edge_b * std::cos(to_radians(degree_ab)));
}

static float DeltaHS_BBS(float h1, float s1, float v1, float h2, float s2, float v2)
{
    float h1_rad = to_radians(h1);
    float h2_rad = to_radians(h2);

    float dx = std::cos(h1_rad) * s1 * v1 - cos(h2_rad) * s2 * v2;
    float dy = std::sin(h1_rad) * s1 * v1 - sin(h2_rad) * s2 * v2;
    float dxy = std::sqrt(dx * dx + dy * dy);
    return std::min(1.2f, dxy);
}

FlushVolCalculator::FlushVolCalculator(int min, int max, float multiplier)
    :m_min_flush_vol(min), m_max_flush_vol(max), m_multiplier(multiplier)
{
}

int FlushVolCalculator::calc_flush_vol(unsigned char src_a, unsigned char src_r, unsigned char src_g, unsigned char src_b,
    unsigned char dst_a, unsigned char dst_r, unsigned char dst_g, unsigned char dst_b)
{
    // BBS: Transparent materials are treated as white materials
    if (src_a == 0) {
        src_r = src_g = src_b = 255;
    }
    if (dst_a == 0) {
        dst_r = dst_g = dst_b = 255;
    }

    float src_r_f, src_g_f, src_b_f, dst_r_f, dst_g_f, dst_b_f;
    float from_hsv_h, from_hsv_s, from_hsv_v;
    float to_hsv_h, to_hsv_s, to_hsv_v;

    src_r_f = (float)src_r / 255.f;
    src_g_f = (float)src_g / 255.f;
    src_b_f = (float)src_b / 255.f;
    dst_r_f = (float)dst_r / 255.f;
    dst_g_f = (float)dst_g / 255.f;
    dst_b_f = (float)dst_b / 255.f;

    // Calculate color distance in HSV color space
    RGB2HSV(src_r_f, src_g_f,src_b_f, &from_hsv_h, &from_hsv_s, &from_hsv_v);
    RGB2HSV(dst_r_f, dst_g_f, dst_b_f, &to_hsv_h, &to_hsv_s, &to_hsv_v);
    float hs_dist = DeltaHS_BBS(from_hsv_h, from_hsv_s, from_hsv_v, to_hsv_h, to_hsv_s, to_hsv_v);

    // 1. Color difference is more obvious if the dest color has high luminance
    // 2. Color difference is more obvious if the source color has low luminance
    float from_lumi = get_luminance(src_r_f, src_g_f, src_b_f);
    float to_lumi = get_luminance(dst_r_f, dst_g_f, dst_b_f);
    float lumi_flush = 0.f;
    if (to_lumi >= from_lumi) {
        lumi_flush = std::pow(to_lumi - from_lumi, 0.7f) * 560.f;
    }
    else {
        lumi_flush = (from_lumi - to_lumi) * 80.f;

        float inter_hsv_v = 0.67 * to_hsv_v + 0.33 * from_hsv_v;
        hs_dist = std::min(inter_hsv_v, hs_dist);
    }
    float hs_flush = 230.f * hs_dist;

    float flush_volume = calc_triangle_3rd_edge(hs_flush, lumi_flush, 120.f);
    flush_volume = std::max(flush_volume, 60.f);

    //float flush_multiplier = std::atof(m_flush_multiplier_ebox->GetValue().c_str());
    flush_volume += m_min_flush_vol;
    return std::min((int)flush_volume, m_max_flush_vol);
}

std::vector<int> get_min_flush_volumes(const DynamicPrintConfig& full_config, size_t physical_nozzle_id)
{
    std::vector<int>extra_flush_volumes;
    //const auto& full_config = wxGetApp().preset_bundle->full_config();
    //auto& printer_config = wxGetApp().preset_bundle->printers.get_edited_preset().config;

    const auto* nozzle_volume_opt = full_config.option<ConfigOptionFloatsNullable>("nozzle_volume");
    const size_t nozzle_volume_idx = nozzle_volume_opt != nullptr && !nozzle_volume_opt->empty() &&
                                         physical_nozzle_id < nozzle_volume_opt->size() ?
                                     physical_nozzle_id : 0;
    int nozzle_volume_val = nozzle_volume_opt != nullptr && !nozzle_volume_opt->empty() &&
                                    !nozzle_volume_opt->is_nil(nozzle_volume_idx) ?
                                int(nozzle_volume_opt->get_at(nozzle_volume_idx)) : 0;

    const ConfigOptionInt* enable_long_retraction_when_cut_opt = full_config.option<ConfigOptionInt>("enable_long_retraction_when_cut");
    int machine_enabled_level = 0;
    if (enable_long_retraction_when_cut_opt) {
        machine_enabled_level = enable_long_retraction_when_cut_opt->value;
        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << boost::format(": get enable_long_retraction_when_cut from config, value=%1%")%machine_enabled_level;
    }
    const ConfigOptionBools* long_retractions_when_cut_opt = full_config.option<ConfigOptionBools>("long_retractions_when_cut");
    bool machine_activated = false;
    if (long_retractions_when_cut_opt && !long_retractions_when_cut_opt->empty()) {
        machine_activated = long_retractions_when_cut_opt->get_at(physical_nozzle_id) == 1;
        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << boost::format(": get long_retractions_when_cut from config, value=%1%, activated=%2%")%int(long_retractions_when_cut_opt->get_at(physical_nozzle_id)) %machine_activated;
    }

    size_t filament_size = full_config.option<ConfigOptionFloats>("filament_diameter")->values.size();
    std::vector<double> filament_retraction_distance_when_cut(filament_size, 18.0f), printer_retraction_distance_when_cut(filament_size, 18.0f);
    std::vector<unsigned char> filament_long_retractions_when_cut(filament_size, 0);
    const ConfigOptionFloats* filament_retraction_distances_when_cut_opt = full_config.option<ConfigOptionFloats>("filament_retraction_distances_when_cut");
    if (filament_retraction_distances_when_cut_opt) {
        filament_retraction_distance_when_cut = filament_retraction_distances_when_cut_opt->values;
        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << boost::format(": get filament_retraction_distance_when_cut from config, size=%1%, values=%2%")%filament_retraction_distance_when_cut.size() %filament_retraction_distances_when_cut_opt->serialize();
    }

    const ConfigOptionFloats* printer_retraction_distance_when_cut_opt = full_config.option<ConfigOptionFloats>("retraction_distances_when_cut");
    if (printer_retraction_distance_when_cut_opt) {
        printer_retraction_distance_when_cut = printer_retraction_distance_when_cut_opt->values;
        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << boost::format(": get retraction_distances_when_cut from config, size=%1%, values=%2%")%printer_retraction_distance_when_cut.size() %printer_retraction_distance_when_cut_opt->serialize();
    }

    const ConfigOptionBools* filament_long_retractions_when_cut_opt = full_config.option<ConfigOptionBools>("filament_long_retractions_when_cut");
    if (filament_long_retractions_when_cut_opt) {
        filament_long_retractions_when_cut = filament_long_retractions_when_cut_opt->values;
        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << boost::format(": get filament_long_retractions_when_cut from config, size=%1%, values=%2%")%filament_long_retractions_when_cut.size() %filament_long_retractions_when_cut_opt->serialize();
    }

    for (size_t idx = 0; idx < filament_size; ++idx) {
        int extra_flush_volume = nozzle_volume_val;
        const double printer_retract_length = printer_retraction_distance_when_cut.empty() ? 18.0 :
            printer_retraction_distance_when_cut[std::min(physical_nozzle_id, printer_retraction_distance_when_cut.size() - 1)];
        int retract_length = machine_enabled_level && machine_activated ? int(printer_retract_length) : 0;

        unsigned char filament_activated = filament_long_retractions_when_cut[idx];
        double filament_retract_length = filament_retraction_distance_when_cut[idx];

        if (filament_activated == 0)
            retract_length = 0;
        else if (filament_activated == 1 && machine_enabled_level == LongRectrationLevel::EnableFilament) {
            if (!std::isnan(filament_retract_length))
                retract_length = (int)filament_retraction_distance_when_cut[idx];
            else
                retract_length = int(printer_retract_length);
        }

        extra_flush_volume -= PI * 1.75 * 1.75 / 4 * retract_length;
        extra_flush_volumes.emplace_back(extra_flush_volume);
    }
    return extra_flush_volumes;
}

bool is_support_filament(PresetBundle& preset_bundle, int extruder_id)
{
    auto &filament_presets = preset_bundle.filament_presets;
    auto &filaments        = preset_bundle.filaments;

    if (extruder_id >= filament_presets.size()) return false;

    Slic3r::Preset *filament = filaments.find_preset(filament_presets[extruder_id]);
    if (filament == nullptr) return false;

    Slic3r::ConfigOptionBools *support_option = dynamic_cast<Slic3r::ConfigOptionBools *>(filament->config.option("filament_is_support"));
    if (support_option == nullptr) return false;

    return support_option->get_at(0);
};

int calc_flushing_volume_from_rgb(const Slic3r::ColorRGB& from_, const Slic3r::ColorRGB& to_ ,int min_flush_volume)
{
    Slic3r::FlushVolCalculator calculator(min_flush_volume, Slic3r::g_max_flush_volume);

    return calculator.calc_flush_vol(255, from_.r_uchar(), from_.g_uchar(), from_.b_uchar(),
         255, to_.r_uchar(), to_.g_uchar(), to_.b_uchar());
}

void recalc_flushing_volumes(DynamicPrintConfig& config, PresetBundle& preset_bundle)
{
    ConfigOptionFloats* flush_matrix_opt = config.option<ConfigOptionFloats>("flush_volumes_matrix");
    const ConfigOptionStrings* filament_colours_opt = config.option<ConfigOptionStrings>("filament_colour");
    if (flush_matrix_opt == nullptr || filament_colours_opt == nullptr)
        return;

    const std::vector<std::string>& extruder_colours = filament_colours_opt->values;
    const size_t                    filament_count   = extruder_colours.size();
    const size_t                    nozzle_count     = std::max(1, preset_bundle.get_printer_extruder_count());
    const size_t                    block_size      = filament_count * filament_count;

    // Always start from an empty matrix. In particular, callers importing a
    // third-party project must not retain any values or dimensions from the 3MF.
    std::vector<double> m_matrix(block_size * nozzle_count, 0.0);

    std::vector<Slic3r::ColorRGB> m_colours;
    for (size_t index = 0; index < filament_count; ++index) {
        Slic3r::ColorRGB rgb;
        Slic3r::decode_color(extruder_colours[index], rgb);
        m_colours.push_back(rgb);
    }

    auto&  ams_multi_color_filament = preset_bundle.ams_multi_color_filment;
    std::vector<std::vector<Slic3r::ColorRGB>> multi_colors;

    // Support for multi-color filament
    for (size_t i = 0; i < m_colours.size(); ++i) {
        std::vector<Slic3r::ColorRGB> single_filament;
        if (i < ams_multi_color_filament.size()) {
            if (!ams_multi_color_filament[i].empty()) {
                std::vector<std::string> colors = ams_multi_color_filament[i];
                for (size_t j = 0; j < colors.size(); ++j) {
                    Slic3r::ColorRGB rgb;
                    Slic3r::decode_color(colors[j], rgb);                    
                    single_filament.push_back(rgb);
                }
                multi_colors.push_back(single_filament);
                continue;
            }
        }
        single_filament.push_back(m_colours[i]);
        multi_colors.push_back(single_filament);
    }

    for (size_t nozzle_id = 0; nozzle_id < nozzle_count; ++nozzle_id) {
        const std::vector<int> min_flush_volume = get_min_flush_volumes(config, nozzle_id);
        if (min_flush_volume.size() < filament_count) {
            BOOST_LOG_TRIVIAL(error) << "[FlushVolCalc] Invalid minimum flushing volume data: nozzle_id=" << nozzle_id
                                       << ", size=" << min_flush_volume.size()
                                       << ", filament_count=" << filament_count;
            return;
        }

        const size_t matrix_offset = nozzle_id * block_size;
        for (size_t from_idx = 0; from_idx < multi_colors.size(); ++from_idx) {
            const bool is_from_support = is_support_filament(preset_bundle, int(from_idx));
            for (size_t to_idx = 0; to_idx < multi_colors.size(); ++to_idx) {
                const size_t matrix_index = matrix_offset + filament_count * from_idx + to_idx;
                if (from_idx == to_idx) {
                    m_matrix[matrix_index] = 0;
                    continue;
                }

                const bool is_to_support = is_support_filament(preset_bundle, int(to_idx));
                int flushing_volume = 0;
                if (is_to_support) {
                    flushing_volume = Slic3r::g_flush_volume_to_support;
                } else {
                    for (size_t i = 0; i < multi_colors[from_idx].size(); ++i) {
                        const Slic3r::ColorRGB& from = multi_colors[from_idx][i];
                        for (size_t j = 0; j < multi_colors[to_idx].size(); ++j) {
                            const Slic3r::ColorRGB& to     = multi_colors[to_idx][j];
                            int             volume = calc_flushing_volume_from_rgb(from, to, min_flush_volume[from_idx]);
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

    flush_matrix_opt->values = std::move(m_matrix);
}

void recalc_flushing_volumes(PresetBundle& preset_bundle)
{
    DynamicPrintConfig full_config = preset_bundle.full_config();
    recalc_flushing_volumes(full_config, preset_bundle);

    const ConfigOptionFloats* recalculated_matrix = full_config.option<ConfigOptionFloats>("flush_volumes_matrix");
    ConfigOptionFloats* project_matrix = preset_bundle.project_config.option<ConfigOptionFloats>("flush_volumes_matrix");
    if (recalculated_matrix != nullptr && project_matrix != nullptr)
        project_matrix->values = recalculated_matrix->values;

    // The imported values were discarded, so this remains an automatically
    // managed matrix and may be recalculated again after a colour change.
    if (ConfigOptionBool* changed = preset_bundle.project_config.option<ConfigOptionBool>("flush_volumes_changed"))
        changed->value = false;
}
}
