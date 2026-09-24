// ColorDecomposeRecipe.cpp
//
// Thin adapter layer (薄壳适配) that exposes the closed-source K-M recipe
// engine (F:\cr_close_library\cr_km_recipe) through the existing C++ public
// API in ColorDecomposeRecipe.hpp. The open-source K-M implementation has
// been moved into the closed-source library; this file only translates
// between the C++ types used by ColorDecomposeDialog and the flat C ABI
// exposed by cr_km_recipe.h.
//
// All algorithmic logic, JSON resource loading, base color spectrum lookup
// and physical filament spectrum lookup live in cr_km_recipe. This file
// owns no K-M math, no JSON I/O, and no spectrum data tables.

#include "ColorDecomposeRecipe.hpp"

#include "cr_km_recipe.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <string>
#include <vector>

namespace Slic3r {
namespace {

// ---------- C ABI lifetime helpers ----------

// cr_close_recommend_from_physical_filaments / cr_close_lookup_standard_recipe
// allocate the result buffers (matched_color_hex, components[i].color_hex,
// components[i].base_color, components[i].spectrum, components[]) on the heap
// using _strdup / malloc. The closed-source ABI does not export a free helper,
// so the adapter has to release the buffers with matching primitives.
void free_close_recipe_result(const CRCloseRecipeResult& r)
{
    if (r.matched_color_hex) {
        std::free(const_cast<char*>(r.matched_color_hex));
    }
    if (r.components) {
        for (unsigned int i = 0; i < r.component_count; ++i) {
            const auto& c = r.components[i];
            if (c.color_hex)  std::free(const_cast<char*>(c.color_hex));
            if (c.base_color) std::free(const_cast<char*>(c.base_color));
            if (c.spectrum)   std::free(const_cast<double*>(c.spectrum));
        }
        std::free(const_cast<CRCloseComponent*>(r.components));
    }
}

// Translate a C ABI mode code (0/1/2) back to the C++ enum. The mapping is
// kept identical to the C3D_RECIPE_MODE_* constants in c3d_color_recipe.h.
ColorDecomposeRecipeMode mode_from_abi(int mode)
{
    switch (mode) {
        case 0: return ColorDecomposeRecipeMode::MaterialList;
        case 2: return ColorDecomposeRecipeMode::RYBW;
        case 1:
        default: return ColorDecomposeRecipeMode::CMYW;
    }
}

int mode_to_abi(ColorDecomposeRecipeMode mode)
{
    switch (mode) {
        case ColorDecomposeRecipeMode::MaterialList: return 0;
        case ColorDecomposeRecipeMode::RYBW:         return 2;
        case ColorDecomposeRecipeMode::CMYW:
        default:                                     return 1;
    }
}

// ---------- C ABI -> C++ translation ----------

// Copy a single CRCloseComponent into a C++ ColorDecomposeRecipeComponent.
// Allocates the optional<ReflectanceSpectrum> when the C ABI reports a
// measured spectrum.
ColorDecomposeRecipeComponent component_from_abi(const CRCloseComponent& c)
{
    ColorDecomposeRecipeComponent out;
    out.color_hex      = c.color_hex  ? c.color_hex  : std::string();
    out.base_color     = c.base_color ? c.base_color : std::string();
    out.ratio          = c.ratio;
    out.filament_index = c.filament_index;
    if (c.has_spectrum && c.spectrum) {
        ReflectanceSpectrum s{};
        std::copy(c.spectrum, c.spectrum + KM_SPECTRUM_POINTS, s.begin());
        out.spectrum = std::move(s);
    }
    return out;
}

ColorDecomposeRecipeResult result_from_abi(const CRCloseRecipeResult& r)
{
    ColorDecomposeRecipeResult out;
    if (!r.valid) return out;
    out.valid            = true;
    out.mode             = mode_from_abi(r.mode);
    out.matched_color_hex = r.matched_color_hex ? r.matched_color_hex : std::string();
    out.components.reserve(r.component_count);
    for (unsigned int i = 0; i < r.component_count; ++i) {
        out.components.push_back(component_from_abi(r.components[i]));
    }
    return out;
}

// Pack a std::vector<ColorDecomposePhysicalFilament> into the flat
// CRCloseFilament array expected by cr_close_recommend_from_physical_filaments.
// The CRCloseFilament struct only carries pointers, so we keep the underlying
// spectrum std::array alive in `spectrum_storage` for the duration of the call.
void pack_physical_filaments(const std::vector<ColorDecomposePhysicalFilament>& in,
                             std::vector<CRCloseFilament>& cr_out,
                             std::vector<ReflectanceSpectrum>& spectrum_storage)
{
    cr_out.clear();
    spectrum_storage.clear();
    cr_out.reserve(in.size());
    spectrum_storage.reserve(in.size());

    for (const auto& f : in) {
        CRCloseFilament cf{};
        cf.color_hex      = f.color_hex.c_str();
        cf.name           = f.name.c_str();
        cf.type           = f.type.c_str();
        cf.filament_index = f.filament_index;
        if (f.spectrum.has_value()) {
            spectrum_storage.push_back(*f.spectrum);
            cf.has_spectrum = 1;
            cf.spectrum     = spectrum_storage.back().data();
        } else {
            cf.has_spectrum = 0;
            cf.spectrum     = nullptr;
        }
        cr_out.push_back(cf);
    }
}

} // namespace

// ---------- Public C++ API (thin wrappers) ----------

std::string color_decompose_rgb_to_hex(const ColorDecomposeRgb& rgb)
{
    char buf[8];
    std::snprintf(buf, sizeof(buf), "#%02X%02X%02X",
                  static_cast<unsigned>(rgb.r),
                  static_cast<unsigned>(rgb.g),
                  static_cast<unsigned>(rgb.b));
    return std::string(buf);
}

bool color_decompose_hex_to_rgb(const std::string& hex, ColorDecomposeRgb& out)
{
    C3dkmSrgb rgb{};
    if (c3dkm_hex_to_rgb(hex.c_str(), &rgb) != 1) return false;
    out.r = rgb.r;
    out.g = rgb.g;
    out.b = rgb.b;
    return true;
}

ColorDecomposeRecipeResult recommend_from_physical_filaments(
    const ColorDecomposeRgb& target,
    const std::vector<ColorDecomposePhysicalFilament>& physical_filaments,
    const std::string& preferred_material_type)
{
    std::vector<CRCloseFilament>       cr_filaments;
    std::vector<ReflectanceSpectrum>   spectrum_storage;
    pack_physical_filaments(physical_filaments, cr_filaments, spectrum_storage);

    CRCloseRecipeResult cr_result{};
    int rc = cr_close_recommend_from_physical_filaments(
        target.r, target.g, target.b,
        cr_filaments.empty() ? nullptr : cr_filaments.data(),
        static_cast<unsigned int>(cr_filaments.size()),
        preferred_material_type.empty() ? nullptr : preferred_material_type.c_str(),
        &cr_result);
    if (rc != 0) {
        free_close_recipe_result(cr_result);
        return ColorDecomposeRecipeResult{};
    }
    ColorDecomposeRecipeResult out = result_from_abi(cr_result);
    free_close_recipe_result(cr_result);
    return out;
}

ColorDecomposeRecipeResult lookup_standard_recipe(
    const ColorDecomposeRgb& target,
    ColorDecomposeRecipeMode mode,
    const std::string& preferred_material_type)
{
    CRCloseRecipeResult cr_result{};
    int rc = cr_close_lookup_standard_recipe(
        target.r, target.g, target.b,
        mode_to_abi(mode),
        preferred_material_type.empty() ? nullptr : preferred_material_type.c_str(),
        &cr_result);
    if (rc != 0) {
        free_close_recipe_result(cr_result);
        return ColorDecomposeRecipeResult{};
    }
    ColorDecomposeRecipeResult out = result_from_abi(cr_result);
    free_close_recipe_result(cr_result);
    return out;
}

bool has_base_color_spectrum(ColorDecomposeRecipeMode mode,
                             const std::string& material,
                             const std::string& base_color)
{
    if (material.empty() || base_color.empty()) return false;
    return cr_close_has_base_color_spectrum(
        mode_to_abi(mode), material.c_str(), base_color.c_str()) == 1;
}

} // namespace Slic3r
