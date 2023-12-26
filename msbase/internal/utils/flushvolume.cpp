#include "msbase/utils/flushvolume.h"
#define M_PI       3.14159265358979323846   // pi
namespace msbase
{
    //static const int m_max_flush_volume = 750.f;
    static const int g_min_flush_volume_from_support = 420.f;
    static const int g_flush_volume_to_support = 230;
    static const float g_min_flush_multiplier = 0.f;
    static const float g_max_flush_multiplier = 3.f;
    const int m_min_flush_volume = 107;
    const int m_max_flush_volume = 800;

    static float to_radians(float degree)
    {
        return degree / 180.f * M_PI;
    }

    float DeltaHS_BBS(float h1, float s1, float v1, float h2, float s2, float v2)
    {
        float h1_rad = to_radians(h1);
        float h2_rad = to_radians(h2);

        float dx = std::cos(h1_rad) * s1 * v1 - cos(h2_rad) * s2 * v2;
        float dy = std::sin(h1_rad) * s1 * v1 - sin(h2_rad) * s2 * v2;
        float dxy = std::sqrt(dx * dx + dy * dy);
        return std::min(1.2f, dxy);
    }


    // The input r, g, b values should be in range [0, 1]. The output h is in range [0, 360], s is in range [0, 1] and v is in range [0, 1].
    void RGB2HSV(float r, float g, float b, float* h, float* s, float* v)
    {
        float Cmax = std::max(std::max(r, g), b);
        float Cmin = std::min(std::min(r, g), b);
        float delta = Cmax - Cmin;

        if (std::abs(delta) < 0.001) {
            *h = 0.f;
        }
        else if (Cmax == r) {
            *h = 60.f * fmod((g - b) / delta, 6.f);
        }
        else if (Cmax == g) {
            *h = 60.f * ((b - r) / delta + 2);
        }
        else {
            *h = 60.f * ((r - g) / delta + 4);
        }

        if (std::abs(Cmax) < 0.001) {
            *s = 0.f;
        }
        else {
            *s = delta / Cmax;
        }

        *v = Cmax;
    }

    static float get_luminance(float r, float g, float b)
    {
        return r * 0.3 + g * 0.59 + b * 0.11;
    }

    static float calc_triangle_3rd_edge(float edge_a, float edge_b, float degree_ab)
    {
        return std::sqrt(edge_a * edge_a + edge_b * edge_b - 2 * edge_a * edge_b * std::cos(to_radians(degree_ab)));
    }

    int calc_flushing_volume(const trimesh::vec3& from, const trimesh::vec3& to)
    {
        float from_hsv_h, from_hsv_s, from_hsv_v;
        float to_hsv_h, to_hsv_s, to_hsv_v;

        // Calculate color distance in HSV color space
        RGB2HSV((float)from.x / 255.f, (float)from.y / 255.f, (float)from.z / 255.f, &from_hsv_h, &from_hsv_s, &from_hsv_v);
        RGB2HSV((float)to.x / 255.f, (float)to.y / 255.f, (float)to.z / 255.f, &to_hsv_h, &to_hsv_s, &to_hsv_v);
        float hs_dist = DeltaHS_BBS(from_hsv_h, from_hsv_s, from_hsv_v, to_hsv_h, to_hsv_s, to_hsv_v);

        // 1. Color difference is more obvious if the dest color has high luminance
        // 2. Color difference is more obvious if the source color has low luminance
        float from_lumi = get_luminance((float)from.x / 255.f, (float)from.y / 255.f, (float)from.z / 255.f);
        float to_lumi = get_luminance((float)to.x / 255.f, (float)to.y / 255.f, (float)to.z / 255.f);
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
        flush_volume += m_min_flush_volume;
        return std::min((int)flush_volume, m_max_flush_volume);
    }

    void calc_flushing_volumes(std::vector<trimesh::vec3>& m_colours, std::vector<bool>& filament_is_support,std::vector<float>& m_matrix,float flush_multiplier)
    {
        int _size = m_colours.size();
        if (_size <= 0 )
        {
            return;
        }

        if (filament_is_support.empty() || filament_is_support.size() < _size)
        {
            filament_is_support.resize(_size, false);
        }

        if (m_matrix.size() != _size * _size)
        {
            m_matrix.resize(_size * _size,0.0f);
        }

        for (int from_idx = 0; from_idx < m_colours.size(); from_idx++) {
            const trimesh::vec3& from = m_colours[from_idx];
            bool is_from_support = filament_is_support.at(from_idx);
            for (int to_idx = 0; to_idx < m_colours.size(); to_idx++) {
                bool is_to_support = filament_is_support.at(to_idx);
                if (from_idx == to_idx) {
                    //m_matrix[to_idx][from_idx] = 0.0f;
                    m_matrix[from_idx * _size + to_idx] = 0.0f;
                }
                else {
                    int flushing_volume = 0;
                    if (is_to_support) {
                        flushing_volume = g_flush_volume_to_support;
                    }
                    else {
                        const trimesh::vec3& to = m_colours[to_idx];
                        flushing_volume = calc_flushing_volume(from, to);
                        if (is_from_support) {
                            flushing_volume = std::max(g_min_flush_volume_from_support, flushing_volume);
                        }
                    }
                    flushing_volume = int(flushing_volume * flush_multiplier);
                    m_matrix[_size * from_idx + to_idx] = flushing_volume;

                }
            }
        }
    }
}

