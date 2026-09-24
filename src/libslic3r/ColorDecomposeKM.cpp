#include "ColorDecomposeKM.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace Slic3r {

namespace {

// CIE grid: 380-780nm, 10nm interval, 41 points.
constexpr double CIE_WL_START = 380.0;
constexpr double CIE_WL_STEP = 10.0;

// Reflectance grid (CSV input): 360-780nm, 10nm interval, 43 points.
constexpr double CSV_WL_START = 360.0;
constexpr double CSV_WL_STEP = 10.0;

// CIE 1931 2 degree standard observer color matching functions.
// Source: CIE 018:2019 Table 6. Tabulated on 380-780nm grid, 10nm interval.
const double CMF_X[KM_CIE_POINTS] = {
    0.0014, 0.0042, 0.0143, 0.0435, 0.1344, 0.2839, 0.3483, 0.3362,
    0.2908, 0.1954, 0.0956, 0.0320, 0.0049, 0.0093, 0.0633, 0.1655,
    0.2904, 0.4334, 0.5945, 0.7621, 0.9163, 1.0263, 1.0622, 1.0026,
    0.8544, 0.6424, 0.4479, 0.2835, 0.1649, 0.0874, 0.0468, 0.0227,
    0.0114, 0.0058, 0.0029, 0.0014, 0.0007, 0.0003, 0.0002, 0.0001, 0.0000
};
const double CMF_Y[KM_CIE_POINTS] = {
    0.0000, 0.0001, 0.0004, 0.0012, 0.0040, 0.0116, 0.0230, 0.0380,
    0.0600, 0.0910, 0.1390, 0.2080, 0.3230, 0.5030, 0.7100, 0.8620,
    0.9540, 0.9950, 0.9950, 0.9520, 0.8700, 0.7570, 0.6310, 0.5030,
    0.3810, 0.2650, 0.1750, 0.1070, 0.0610, 0.0320, 0.0170, 0.0082,
    0.0041, 0.0021, 0.0010, 0.0005, 0.0002, 0.0001, 0.0001, 0.0000, 0.0000
};
const double CMF_Z[KM_CIE_POINTS] = {
    0.0065, 0.0201, 0.0679, 0.2074, 0.6456, 1.3856, 1.7471, 1.7721,
    1.6692, 1.2876, 0.8130, 0.4652, 0.2720, 0.1582, 0.0782, 0.0422,
    0.0203, 0.0087, 0.0039, 0.0021, 0.0017, 0.0011, 0.0008, 0.0003,
    0.0002, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000,
    0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000
};

// CIE D65 standard illuminant relative spectral power distribution.
// Source: ISO/CIE 11664-2:2022. 380-780nm, 10nm interval, 41 points.
const double D65_SPD[KM_CIE_POINTS] = {
    49.9755, 54.6482, 82.7549, 91.486, 93.4318, 86.6823, 104.865,
    117.008, 117.812, 114.861, 115.923, 108.811, 109.354, 107.802,
    104.79, 107.689, 104.405, 104.046, 100.0, 96.3342,
    95.788, 88.6856, 90.0062, 89.5991,
    87.6987, 83.2886, 83.6992, 80.0268, 80.2146, 82.2778, 78.2842, 69.7213,
    71.6091, 74.349, 61.604, 69.8856, 75.087, 63.5927, 46.4182, 66.8054,
    63.3828
};

// D65 reference white point (2 degree observer).
constexpr double WHITE_D65_X = 95.047;
constexpr double WHITE_D65_Y = 100.000;
constexpr double WHITE_D65_Z = 108.883;

// Linear interpolate the 43-point CSV spectrum onto the 41-point CIE grid.
// CIE grid starts at 380nm which corresponds to the 3rd CSV point (index 2).
ReflectanceSpectrum interpolate_to_cie_grid(const ReflectanceSpectrum& s)
{
    ReflectanceSpectrum out{};
    // CSV indices 2..42 correspond to 380..780nm, 10nm apart. The CIE grid
    // also starts at 380nm. We index by wavelength for clarity.
    for (int i = 0; i < KM_CIE_POINTS; ++i) {
        const double wl = CIE_WL_START + i * CIE_WL_STEP;
        const double pos = (wl - CSV_WL_START) / CSV_WL_STEP;
        const int    lo  = static_cast<int>(std::floor(pos));
        const int    hi  = lo + 1;
        if (hi >= KM_SPECTRUM_POINTS) {
            out[static_cast<size_t>(i)] = s[static_cast<size_t>(lo)];
            continue;
        }
        if (lo < 0) {
            out[static_cast<size_t>(i)] = s[0];
            continue;
        }
        const double t = pos - lo;
        out[static_cast<size_t>(i)] = s[static_cast<size_t>(lo)] * (1.0 - t) + s[static_cast<size_t>(hi)] * t;
    }
    return out;
}

double srgb_unit_to_linear(double v)
{
    if (v <= 0.04045) return v / 12.92;
    return std::pow((v + 0.055) / 1.055, 2.4);
}

double linear_to_srgb_unit(double v)
{
    if (v <= 0.0031308) return 12.92 * v;
    return 1.055 * std::pow(v, 1.0 / 2.4) - 0.055;
}

double clamp01(double v)
{
    if (v < 0.0) return 0.0;
    if (v > 1.0) return 1.0;
    return v;
}

double lab_f(double t)
{
    return t > 0.008856 ? std::cbrt(t) : 7.787 * t + 16.0 / 116.0;
}

double lab_f_inv(double t)
{
    return t > 0.206897 ? t * t * t : (t - 16.0 / 116.0) / 7.787;
}

} // namespace

// ----- K-M core -----

double km_function(double r_fraction)
{
    if (r_fraction < 1e-6) r_fraction = 1e-6;
    if (r_fraction > 1.0)  r_fraction = 1.0;
    return (1.0 - r_fraction) * (1.0 - r_fraction) / (2.0 * r_fraction);
}

double km_inverse(double ks)
{
    if (ks < 0.0) ks = 0.0;
    return 1.0 + ks - std::sqrt(ks * ks + 2.0 * ks);
}

ReflectanceSpectrum km_mix_spectra(const std::vector<ReflectanceSpectrum>& spectra,
                                   const std::vector<int>& weights)
{
    ReflectanceSpectrum out{};
    if (spectra.empty() || spectra.size() != weights.size()) {
        return out;
    }
    long total = 0;
    for (int w : weights) {
        if (w > 0) total += w;
    }
    if (total <= 0) {
        return out;
    }

    for (size_t i = 0; i < KM_SPECTRUM_POINTS; ++i) {
        double ks_mix = 0.0;
        for (size_t j = 0; j < spectra.size(); ++j) {
            const double w = weights[j] > 0 ? static_cast<double>(weights[j]) : 0.0;
            if (w <= 0.0) continue;
            const double r_pct = spectra[j][i];
            const double r     = clamp01(r_pct / 100.0);
            ks_mix += (w / static_cast<double>(total)) * km_function(r);
        }
        out[i] = clamp01(km_inverse(ks_mix)) * 100.0;
    }
    return out;
}

// ----- Color space conversion -----

XyzColor reflectance_to_xyz(const ReflectanceSpectrum& spectrum)
{
    const ReflectanceSpectrum r_cie = interpolate_to_cie_grid(spectrum);
    // k = 100 / sum(S * Y)
    double sy_sum = 0.0;
    for (int i = 0; i < KM_CIE_POINTS; ++i) {
        sy_sum += D65_SPD[i] * CMF_Y[i];
    }
    const double k = (sy_sum > 0.0) ? (100.0 / sy_sum) : 1.0;

    XyzColor xyz;
    for (int i = 0; i < KM_CIE_POINTS; ++i) {
        const double r = r_cie[static_cast<size_t>(i)] / 100.0;
        xyz.x += r * D65_SPD[i] * CMF_X[i];
        xyz.y += r * D65_SPD[i] * CMF_Y[i];
        xyz.z += r * D65_SPD[i] * CMF_Z[i];
    }
    xyz.x *= k;
    xyz.y *= k;
    xyz.z *= k;
    return xyz;
}

LabColor xyz_to_lab(const XyzColor& xyz)
{
    const double fx = lab_f(xyz.x / WHITE_D65_X);
    const double fy = lab_f(xyz.y / WHITE_D65_Y);
    const double fz = lab_f(xyz.z / WHITE_D65_Z);
    return LabColor{ 116.0 * fy - 16.0,
                     500.0 * (fx - fy),
                     200.0 * (fy - fz) };
}

SRgb lab_to_srgb(const LabColor& lab)
{
    const double fy = (lab.l + 16.0) / 116.0;
    const double fx = lab.a / 500.0 + fy;
    const double fz = fy - lab.b / 200.0;
    const double x  = lab_f_inv(fx) * WHITE_D65_X;
    const double y  = lab_f_inv(fy) * WHITE_D65_Y;
    const double z  = lab_f_inv(fz) * WHITE_D65_Z;

    // Linear RGB via XYZ matrix (D65).
    double rL =  3.2406 * x / 100.0 - 1.5372 * y / 100.0 - 0.4986 * z / 100.0;
    double gL = -0.9689 * x / 100.0 + 1.8758 * y / 100.0 + 0.0415 * z / 100.0;
    double bL =  0.0557 * x / 100.0 - 0.2040 * y / 100.0 + 1.0570 * z / 100.0;

    rL = linear_to_srgb_unit(clamp01(rL));
    gL = linear_to_srgb_unit(clamp01(gL));
    bL = linear_to_srgb_unit(clamp01(bL));

    SRgb out;
    out.r = static_cast<std::uint8_t>(clamp01(rL) * 255.0 + 0.5);
    out.g = static_cast<std::uint8_t>(clamp01(gL) * 255.0 + 0.5);
    out.b = static_cast<std::uint8_t>(clamp01(bL) * 255.0 + 0.5);
    return out;
}

std::string rgb_to_hex(const SRgb& rgb)
{
    char buf[8];
    std::snprintf(buf, sizeof(buf), "#%02X%02X%02X", rgb.r, rgb.g, rgb.b);
    return std::string(buf);
}

bool hex_to_rgb(const std::string& hex, SRgb& out)
{
    if (hex.size() < 7 || hex[0] != '#') return false;
    unsigned r = 0, g = 0, b = 0;
    if (std::sscanf(hex.c_str(), "#%02x%02x%02x", &r, &g, &b) != 3) return false;
    out.r = static_cast<std::uint8_t>(r);
    out.g = static_cast<std::uint8_t>(g);
    out.b = static_cast<std::uint8_t>(b);
    return true;
}

LabColor srgb_to_lab(const SRgb& rgb)
{
    const double rL = srgb_unit_to_linear(rgb.r / 255.0);
    const double gL = srgb_unit_to_linear(rgb.g / 255.0);
    const double bL = srgb_unit_to_linear(rgb.b / 255.0);
    XyzColor xyz;
    xyz.x = (0.4124564 * rL + 0.3575761 * gL + 0.1804375 * bL) * 100.0;
    xyz.y = (0.2126729 * rL + 0.7151522 * gL + 0.0721750 * bL) * 100.0;
    xyz.z = (0.0193339 * rL + 0.1191920 * gL + 0.9503041 * bL) * 100.0;
    return xyz_to_lab(xyz);
}

double delta_e76(const LabColor& a, const LabColor& b)
{
    const double dl = a.l - b.l;
    const double da = a.a - b.a;
    const double db = a.b - b.b;
    return std::sqrt(dl * dl + da * da + db * db);
}

std::string km_blend_color_multi_with_spectra(
    const std::vector<std::string>& hex_colors,
    const std::vector<ReflectanceSpectrum>& spectra,
    const std::vector<int>& weights)
{
    if (hex_colors.size() != weights.size() || hex_colors.size() != spectra.size()) {
        return {};
    }
    if (hex_colors.empty()) {
        return "#000000";
    }
    // Drop zero-weight entries while keeping the same length.
    std::vector<ReflectanceSpectrum> eff_spectra;
    std::vector<int>                 eff_weights;
    eff_spectra.reserve(hex_colors.size());
    eff_weights.reserve(hex_colors.size());
    for (size_t i = 0; i < hex_colors.size(); ++i) {
        if (weights[i] > 0) {
            eff_spectra.push_back(spectra[i]);
            eff_weights.push_back(weights[i]);
        }
    }
    if (eff_spectra.empty()) return "#000000";
    if (eff_spectra.size() == 1) {
        // Single component: convert the supplied spectrum directly to hex.
        const XyzColor xyz = reflectance_to_xyz(eff_spectra.front());
        const LabColor lab = xyz_to_lab(xyz);
        return rgb_to_hex(lab_to_srgb(lab));
    }

    const ReflectanceSpectrum mixed = km_mix_spectra(eff_spectra, eff_weights);
    const XyzColor             xyz  = reflectance_to_xyz(mixed);
    const LabColor             lab  = xyz_to_lab(xyz);
    return rgb_to_hex(lab_to_srgb(lab));
}

std::string km_blend_color_multi_hex(const std::vector<std::string>& hex_colors,
                                     const std::vector<int>& weights)
{
    if (hex_colors.size() != weights.size() || hex_colors.empty()) {
        return {};
    }
    // Without supplied spectra, fall back to K-M via sRGB-derived synthetic
    // spectrum. This produces a K-M-style estimate without external data so
    // the engine remains useful even when only hex colors are available.
    std::vector<ReflectanceSpectrum> spectra;
    spectra.reserve(hex_colors.size());
    for (const std::string& h : hex_colors) {
        SRgb rgb;
        if (!hex_to_rgb(h, rgb)) {
            ReflectanceSpectrum zero{};
            spectra.push_back(zero);
            continue;
        }
        const LabColor lab = srgb_to_lab(rgb);
        const SRgb     srgb_back = lab_to_srgb(lab);
        // Treat the sRGB triple as a flat spectrum with the same value at
        // every wavelength. This is a placeholder for future measured
        // spectra and should be replaced whenever a real measurement exists.
        ReflectanceSpectrum s{};
        const double r_pct = (srgb_back.r / 255.0) * 100.0;
        const double g_pct = (srgb_back.g / 255.0) * 100.0;
        const double b_pct = (srgb_back.b / 255.0) * 100.0;
        for (size_t i = 0; i < KM_SPECTRUM_POINTS; ++i) {
            s[i] = (r_pct + g_pct + b_pct) / 3.0;
        }
        spectra.push_back(s);
    }
    return km_blend_color_multi_with_spectra(hex_colors, spectra, weights);
}

} // namespace Slic3r
