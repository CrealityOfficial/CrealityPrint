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

} // namespace Slic3r

#endif // SLIC3R_COLOR_DECOMPOSE_KM_HPP
