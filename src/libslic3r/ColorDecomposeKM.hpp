#ifndef SLIC3R_COLOR_DECOMPOSE_KM_HPP
#define SLIC3R_COLOR_DECOMPOSE_KM_HPP

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace Slic3r {

// Reflectance spectrum in 360-780nm range, 10nm interval, 43 points.
// Values are in percentage (0-100). All operations stay in percent space
// internally and only convert to fractions at the K-M function.
constexpr int KM_SPECTRUM_POINTS = 43;
using ReflectanceSpectrum = std::array<double, KM_SPECTRUM_POINTS>;

// CIE 1931 2 degree standard observer color matching functions and D65 SPD
// are tabulated on 380-780nm grid, 10nm interval, 41 points. The mixing
// step interpolates the 43-point spectrum onto this 41-point grid.
constexpr int KM_CIE_POINTS = 41;

struct LabColor {
    double l{0.0};
    double a{0.0};
    double b{0.0};
};

struct XyzColor {
    double x{0.0};
    double y{0.0};
    double z{0.0};
};

// ----- K-M engine: core pigment mixing (matches color_mixer.py) -----

// K/S = (1 - R) ^ 2 / (2 R)  with R in [0, 1]
double km_function(double r_fraction);

// R = 1 + K/S - sqrt((K/S)^2 + 2 * K/S); result is in [0, 1]
double km_inverse(double ks);

// Mix N reflectance spectra by integer weights. The K/S value at every
// wavelength is the weighted average of the per-material K/S values:
//   (K/S)_mix(lambda) = sum_i (w_i / W) * (K/S)_i(lambda)
// Returns the resulting reflectance in percent (0-100).
ReflectanceSpectrum km_mix_spectra(const std::vector<ReflectanceSpectrum>& spectra,
                                  const std::vector<int>& weights);

// ----- Color space conversion (matches color_mixer.py) -----

// Convert a 43-point (360-780nm) spectrum to CIE XYZ (D65 / 2 degree).
// Returns Y in [0, 100] scale.
XyzColor reflectance_to_xyz(const ReflectanceSpectrum& spectrum);

// Convert CIE XYZ (D65 / 2 degree) to CIELAB.
LabColor xyz_to_lab(const XyzColor& xyz);

// Convert CIELAB to sRGB in [0, 255]. Out-of-gamut values are clamped.
struct SRgb {
    std::uint8_t r{0};
    std::uint8_t g{0};
    std::uint8_t b{0};
};
SRgb lab_to_srgb(const LabColor& lab);

// Hex helpers.
std::string rgb_to_hex(const SRgb& rgb);
bool        hex_to_rgb(const std::string& hex, SRgb& out);

// Convert sRGB to CIELAB (D65 / 2 degree). sRGB components are 0-255.
LabColor srgb_to_lab(const SRgb& rgb);

// CIE76 color difference (Euclidean distance in Lab).
double delta_e76(const LabColor& a, const LabColor& b);

// High-level helper: hex in, mixed hex out, mixing is done via K-M.
// Returns empty string when inputs are invalid.
std::string km_blend_color_multi_hex(const std::vector<std::string>& hex_colors,
                                     const std::vector<int>& weights);

// Same as above but caller supplies the spectra directly. When any spectrum
// is missing (size 0) the entry is treated as the matching hex color and
// falls back to sRGB-to-synthetic-spectrum conversion. A future enhancement
// may replace the synthetic fallback with measured spectra.
std::string km_blend_color_multi_with_spectra(
    const std::vector<std::string>& hex_colors,
    const std::vector<ReflectanceSpectrum>& spectra,
    const std::vector<int>& weights);

// ----- Base-color spectrum lookup (read side of base_color_spectra.json) -----
//
// The write side (bundled cache seeding and network refresh) lives in
// src/slic3r/GUI/BaseColorSpectraCache.cpp; this module only loads
// base_color_spectra.json, indexes it by official color, and answers "does
// this filament color have a measured spectrum". Kept in ColorDecomposeKM
// because it feeds the K-M engine below (same translation unit, no extra
// build-system entry).
namespace BaseColorSpectra {

// Look up the measured reflectance spectrum whose official color matches
// `color_hex` ("#RRGGBB"; a trailing alpha component is ignored). The eight
// Hyper PLA base-color spectra ship in
// resources/filament_mixing/base_color_spectra.json and are mirrored into
// the data directory at startup (see GUI::BaseColorSpectraCache).
//
// Matching is by color only: the cache holds the official product-line
// spectra and any filament whose color equals an official base color reuses
// that measurement. (A material dimension was evaluated and dropped: preset
// filament_type fields record the material family ("PLA") while the cache
// records the product line ("Hyper PLA"), so type matching either never hit
// or degenerated back to color matching. Revisit the hit rule here when the
// spectrum cache grows beyond one product line.)
//
// Colors are matched exactly after normalization, then with a small
// per-channel tolerance, so vendor-provided preset colors still match
// spectra whose official_hex was recomputed from the measurement itself.
//
// The JSON index is loaded lazily on first use and cached in memory; call
// invalidate() after rewriting the cache file.
bool lookup_spectrum_by_color(const std::string& color_hex,
                              ReflectanceSpectrum& out);

// Drop the in-memory index so the next lookup reloads it from disk.
void invalidate();

// Blend N hex colors by integer weights, Kubelka-Munk first: when EVERY
// color matches a measured base-color spectrum, blend via K-M so the
// mixed-color preview agrees with the K-M-based color-decompose
// recommendation; when any color has no matching measured spectrum, fall
// back to the polynomial mixer (filament_mixer) so arbitrary colors keep
// their existing behavior.
std::string blend_color_multi_spectrum_first(const std::vector<std::string>& hex_colors,
                                             const std::vector<int>&        weights);

} // namespace BaseColorSpectra

} // namespace Slic3r

#endif // SLIC3R_COLOR_DECOMPOSE_KM_HPP
