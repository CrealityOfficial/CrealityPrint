#include "ColorDecomposeKM.hpp"

#include "Utils.hpp"
#include "filament_mixer.h"

#include <boost/filesystem.hpp>
#include <boost/log/trivial.hpp>
#include <boost/nowide/fstream.hpp>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

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

// ---------------------------------------------------------------------------
// BaseColorSpectra: read-side lookup for the measured base-color reflectance
// spectra consumed by the K-M mixing engine. Declared in ColorDecomposeKM.hpp;
// kept in this translation unit so the lookup feeds the K-M engine without a
// separate build-system entry.
// ---------------------------------------------------------------------------

namespace BaseColorSpectra {
namespace {

using Json = nlohmann::json;

// Per-channel tolerance (in 8-bit sRGB units) for matching a filament color
// against an official_hex. official_hex values are recomputed from the
// measured spectrum (D65/2deg -> sRGB), so vendor-provided preset colors can
// differ by a rounding step or two; 3 per channel covers that while the
// eight base colors stay far enough apart to never cross-match.
constexpr int kColorMatchTolerance = 3;

struct SpectrumEntry
{
    std::string         hex;       // normalized uppercase "#RRGGBB"
    ReflectanceSpectrum spectrum;
};

struct SpectrumIndex
{
    // Key: normalized hex color. Matching is color-only on purpose: the
    // cache holds the official product-line spectra and any filament whose
    // color equals an official base color reuses that measurement.
    std::unordered_map<std::string, ReflectanceSpectrum> exact;
    // Kept in file order for the tolerant fallback scan.
    std::vector<SpectrumEntry> ordered;
};

std::mutex& index_mutex()
{
    static std::mutex mutex;
    return mutex;
}

bool          g_index_loaded = false;
bool          g_index_load_attempted = false;
SpectrumIndex g_index;

boost::filesystem::path cache_path()
{
    return boost::filesystem::path(data_dir()) / "filament_mixing" / "base_color_spectra.json";
}

boost::filesystem::path bundled_path()
{
    return boost::filesystem::path(resources_dir()) / "filament_mixing" / "base_color_spectra.json";
}

// Normalize "#rrggbb..." (any case, optional alpha) to uppercase "#RRGGBB".
bool normalize_hex(const std::string& in, std::string& out)
{
    if (in.size() < 7 || in.front() != '#')
        return false;
    out.clear();
    out.reserve(7);
    out.push_back('#');
    for (size_t i = 1; i < 7; ++i) {
        const unsigned char c = static_cast<unsigned char>(in[i]);
        if (!std::isxdigit(c))
            return false;
        out.push_back(static_cast<char>(std::toupper(c)));
    }
    return true;
}

bool hex_to_rgb_u8(const std::string& hex7, unsigned char rgb[3])
{
    unsigned r = 0, g = 0, b = 0;
    if (std::sscanf(hex7.c_str(), "#%02x%02x%02x", &r, &g, &b) != 3)
        return false;
    rgb[0] = static_cast<unsigned char>(r);
    rgb[1] = static_cast<unsigned char>(g);
    rgb[2] = static_cast<unsigned char>(b);
    return true;
}

bool read_index_from(const boost::filesystem::path& path, SpectrumIndex& index)
{
    boost::nowide::ifstream stream(path.string());
    if (!stream)
        return false;

    Json root;
    try {
        stream >> root;
    } catch (const std::exception&) {
        return false;
    }

    if (!root.is_object() || !root.contains("entries") || !root["entries"].is_array())
        return false;

    for (const Json& entry : root["entries"]) {
        if (!entry.is_object() || !entry.contains("spectrum") || !entry["spectrum"].is_array())
            continue;
        const auto& spectrum_json = entry["spectrum"];
        if (spectrum_json.size() != KM_SPECTRUM_POINTS)
            continue;

        std::string hex;
        if (!entry.contains("official_hex") || !entry["official_hex"].is_string() ||
            !normalize_hex(entry["official_hex"].get<std::string>(), hex))
            continue;

        // The "material" field (product line, e.g. "Hyper PLA") is parsed
        // for forward compatibility but not used: lookup matches color only.

        ReflectanceSpectrum spectrum{};
        bool valid = true;
        for (size_t i = 0; i < spectrum.size(); ++i) {
            const Json& value = spectrum_json[i];
            if (!value.is_number()) {
                valid = false;
                break;
            }
            const double number = value.get<double>();
            if (!std::isfinite(number) || number < 0.0 || number > 100.0) {
                valid = false;
                break;
            }
            spectrum[i] = number;
        }
        if (!valid)
            continue;

        index.exact.emplace(hex, spectrum);
        index.ordered.push_back(SpectrumEntry{hex, spectrum});
    }

    return !index.ordered.empty();
}

// Loads (once) the spectrum index from the data-dir cache, falling back to
// the bundled resource. Must be called with index_mutex() held.
bool ensure_index_locked()
{
    if (g_index_loaded)
        return true;
    if (g_index_load_attempted)
        return false;
    g_index_load_attempted = true;

    SpectrumIndex index;
    if (!read_index_from(cache_path(), index) && !read_index_from(bundled_path(), index)) {
        BOOST_LOG_TRIVIAL(warning) << "BaseColorSpectra: no usable base_color_spectra.json; "
                                   << "mixed-color preview falls back to the polynomial mixer";
        return false;
    }

    g_index = std::move(index);
    g_index_loaded = true;
    return true;
}

} // namespace

bool lookup_spectrum_by_color(const std::string& color_hex,
                              ReflectanceSpectrum& out)
{
    std::string key;
    if (!normalize_hex(color_hex, key))
        return false;

    std::lock_guard<std::mutex> lock(index_mutex());
    if (!ensure_index_locked())
        return false;

    const auto it = g_index.exact.find(key);
    if (it != g_index.exact.end()) {
        out = it->second;
        return true;
    }

    // Tolerant scan: official_hex values are recomputed from the measured
    // spectrum, so vendor-provided preset colors can differ by a rounding
    // step or two.
    unsigned char want[3];
    if (!hex_to_rgb_u8(key, want))
        return false;
    for (const SpectrumEntry& e : g_index.ordered) {
        unsigned char have[3];
        if (!hex_to_rgb_u8(e.hex, have))
            continue;
        if (std::abs(int(want[0]) - int(have[0])) <= kColorMatchTolerance &&
            std::abs(int(want[1]) - int(have[1])) <= kColorMatchTolerance &&
            std::abs(int(want[2]) - int(have[2])) <= kColorMatchTolerance) {
            out = e.spectrum;
            return true;
        }
    }
    return false;
}

void invalidate()
{
    std::lock_guard<std::mutex> lock(index_mutex());
    g_index_loaded = false;
    g_index_load_attempted = false;
    g_index = SpectrumIndex{};
}

std::string blend_color_multi_spectrum_first(const std::vector<std::string>& hex_colors,
                                             const std::vector<int>&        weights)
{
    // Degenerate inputs keep the polynomial mixer's existing behavior
    // (including its "#000000" / single-color conventions).
    if (hex_colors.size() != weights.size() || hex_colors.size() < 2)
        return blend_color_multi(hex_colors, weights);

    std::vector<ReflectanceSpectrum> spectra;
    spectra.reserve(hex_colors.size());
    for (size_t i = 0; i < hex_colors.size(); ++i) {
        ReflectanceSpectrum spectrum{};
        if (!lookup_spectrum_by_color(hex_colors[i], spectrum))
            return blend_color_multi(hex_colors, weights);
        spectra.push_back(spectrum);
    }

    const std::string mixed = km_blend_color_multi_with_spectra(hex_colors, spectra, weights);
    return mixed.empty() ? blend_color_multi(hex_colors, weights) : mixed;
}

} // namespace BaseColorSpectra

} // namespace Slic3r
