#ifndef SLIC3R_COLOR_DECOMPOSE_RECIPE_HPP
#define SLIC3R_COLOR_DECOMPOSE_RECIPE_HPP

#include <optional>
#include <string>
#include <vector>

#include "ColorDecomposeKM.hpp"

namespace Slic3r {

enum class ColorDecomposeRecipeMode {
    MaterialList,
    CMYW,
    RYBW
};

struct ColorDecomposeRgb {
    unsigned char r{0};
    unsigned char g{0};
    unsigned char b{0};
};

struct ColorDecomposePhysicalFilament {
    std::string  color_hex;
    std::string  name;
    std::string  type;
    bool         is_mixed{false};
    unsigned int filament_index{0}; // 1-based physical filament index
    // Optional K-M reflectance spectrum. When set, recommend_from_physical_filaments
    // can use the K-M engine for this filament. When empty the polynomial mixer is
    // used for this candidate.
    std::optional<ReflectanceSpectrum> spectrum;
};

struct ColorDecomposeRecipeComponent {
    std::string  color_hex;
    std::string  base_color;
    int          ratio{0};
    unsigned int filament_index{0}; // 1-based for physical filaments, 0 for standard base colors
    // Optional K-M spectrum captured alongside the component, used by the
    // K-M path to mix and by the fallback path for diagnostics.
    std::optional<ReflectanceSpectrum> spectrum;
};

struct ColorDecomposeRecipeResult {
    bool valid{false};
    ColorDecomposeRecipeMode mode{ColorDecomposeRecipeMode::MaterialList};
    std::string matched_color_hex;
    std::vector<ColorDecomposeRecipeComponent> components;
};

std::string color_decompose_rgb_to_hex(const ColorDecomposeRgb& rgb);
bool color_decompose_hex_to_rgb(const std::string& hex, ColorDecomposeRgb& out);

ColorDecomposeRecipeResult recommend_from_physical_filaments(
    const ColorDecomposeRgb& target,
    const std::vector<ColorDecomposePhysicalFilament>& physical_filaments,
    const std::string& preferred_material_type);

// K-M based lookup. Requires base_color_spectra.json to contain entries for
// the requested mode and material. Both CMYW and RYBW modes are supported
// when their base color spectra (Cyan/Magenta/Yellow/White or Red/Blue/
// Yellow/White) are present in base_color_spectra.json; otherwise the
// function returns an invalid result.
ColorDecomposeRecipeResult lookup_standard_recipe(
    const ColorDecomposeRgb& target,
    ColorDecomposeRecipeMode mode,
    const std::string& preferred_material_type);

// Returns true if base_color_spectra.json has at least one entry for the
// requested (mode, material, base_color) combination.
bool has_base_color_spectrum(ColorDecomposeRecipeMode mode,
                             const std::string& material,
                             const std::string& base_color);

} // namespace Slic3r

#endif // SLIC3R_COLOR_DECOMPOSE_RECIPE_HPP
