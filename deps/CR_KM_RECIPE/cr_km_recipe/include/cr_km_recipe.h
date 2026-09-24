/*
 * cr_km_recipe.h - single public C ABI for the closed-source K-M recipe
 * library.
 *
 * This header is the only file a consumer needs to include. It consolidates
 * the previous three headers (c3dkm.h / c3d_color_recipe.h /
 * cr_close_library.h) into one place. Three layers of API are exposed, in
 * order of increasing abstraction:
 *
 *   1. Low-level K-M math + color-space conversion  (c3dkm_*)
 *   2. Mid-level recipe evaluation                   (c3d_recipe_*)
 *   3. Caller-friendly structured-result wrappers   (cr_close_*)
 *
 * The high-level layer (3) is what C3DSlicer's ColorDecomposeRecipe.cpp uses.
 * Layers (1) and (2) are kept for diagnostics, scripting tools and the
 * library's own internal tests.
 *
 * All functions return 0 on success and non-zero on error. All output buffers
 * are caller-allocated.
 */
#ifndef CR_KM_RECIPE_H
#define CR_KM_RECIPE_H

#include <cstdint>
#include <cstddef>

/* ---------------------------------------------------------------------------
 * Export-macro selection.
 *
 *   CR_CLOSE_STATIC_DEFINE   defined  -> CR_CLOSE_API is empty (the consumer
 *                                       links against the static archive,
 *                                       symbols come from .obj files).
 *   CR_CLOSE_LIBRARY_EXPORTS defined  -> CR_CLOSE_API is __declspec(dllexport)
 *                                       (the library is being built).
 *   otherwise                           -> __declspec(dllimport) on Windows,
 *                                       __attribute__((visibility("default")))
 *                                       on Linux/macOS.
 * ------------------------------------------------------------------------- */
#ifdef CR_CLOSE_STATIC_DEFINE
    #define CR_CLOSE_API
#elif defined(_WIN32)
    #ifdef CR_CLOSE_LIBRARY_EXPORTS
        #define CR_CLOSE_API __declspec(dllexport)
    #else
        #define CR_CLOSE_API __declspec(dllimport)
    #endif
#else
    #define CR_CLOSE_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ===========================================================================
 * Layer 1 - K-M math + color-space conversion.
 *
 * Spectra are 43 doubles on a 360..780nm grid, 10nm interval. The CIE grid
 * is 41 points (380..780nm); the engine performs the linear interpolation
 * internally.
 * ======================================================================== */

#define C3DKM_SPECTRUM_POINTS 43
#define C3DKM_CIE_POINTS      41

/* Basic color types (passed across the ABI as plain structs of doubles). */
typedef struct {
    double v[C3DKM_SPECTRUM_POINTS];
} C3dkmSpectrum;

typedef struct {
    double x, y, z;
} C3dkmXyz;

typedef struct {
    double L, a, b;
} C3dkmLab;

typedef struct {
    unsigned char r, g, b;
} C3dkmSrgb;

/* --- Core K-M math ----------------------------------------------------- */

CR_CLOSE_API double c3dkm_function(double r_fraction);
CR_CLOSE_API double c3dkm_inverse(double ks);

/* out_spectrum must point to C3DKM_SPECTRUM_POINTS doubles. */
CR_CLOSE_API int c3dkm_mix_spectra(const C3dkmSpectrum* spectra,
                                   const int* weights,
                                   unsigned int count,
                                   double* out_spectrum);

/* --- Color space conversion ------------------------------------------- */

CR_CLOSE_API C3dkmXyz c3dkm_reflectance_to_xyz(const double* spectrum);
CR_CLOSE_API C3dkmLab c3dkm_xyz_to_lab(C3dkmXyz xyz);
CR_CLOSE_API C3dkmSrgb c3dkm_lab_to_srgb(C3dkmLab lab);
CR_CLOSE_API C3dkmLab c3dkm_srgb_to_lab(C3dkmSrgb rgb);
CR_CLOSE_API double   c3dkm_delta_e76(C3dkmLab a, C3dkmLab b);

/* --- Convenience ------------------------------------------------------ */

/* out_hex must point to at least 8 bytes. Returns 0 on success. */
CR_CLOSE_API int c3dkm_rgb_to_hex(C3dkmSrgb rgb, char* out_hex);

/* Returns 1 on success and fills out_rgb. 0 on parse failure. */
CR_CLOSE_API int c3dkm_hex_to_rgb(const char* hex, C3dkmSrgb* out_rgb);

/* ===========================================================================
 * Layer 2 - Mid-level recipe evaluation.
 *
 * Uses caller-allocated buffers; the host translates its own std::string /
 * std::vector types when invoking these entry points.
 * ======================================================================== */

/* Mode codes shared with the higher level dialogs. Keep in sync with the
 * C++ ColorDecomposeRecipeMode enum in the host project. */
#define C3D_RECIPE_MODE_MATERIAL_LIST 0
#define C3D_RECIPE_MODE_CMYW          1
#define C3D_RECIPE_MODE_RYBW          2

struct C3dRecipeComponent {
    int          ratio;          /* integer 0..100                            */
    unsigned int filament_index; /* 1-based physical; 0 for standard base     */
    char         color_hex[8];   /* "#RRGGBB\0"                                */
    char         base_color[16]; /* "Cyan", "Magenta", ... or empty            */
    int          has_spectrum;   /* 0 / 1                                      */
    double       spectrum[C3DKM_SPECTRUM_POINTS];
};

struct C3dRecipeResult {
    int  valid;             /* 0 / 1                                  */
    int  mode;              /* C3D_RECIPE_MODE_*                      */
    char matched_color_hex[8];
    unsigned int component_count;
    /* Caller-allocated; sized by the recipe's worst case (4 components). */
    C3dRecipeComponent* components;
    unsigned int        components_capacity;
};

/* Host populates a C3dRecipeResult with `components` pointing to caller
 * memory of at least `capacity` entries. The engine writes at most 4 entries
 * and updates `component_count`. Returns 0 on success.
 *
 * target_rgb in 0..255. preferred_material_type may be NULL.
 * filaments: array of `filament_count` entries. Each entry's `spectrum` may
 * be NULL.
 */
CR_CLOSE_API int c3d_recipe_recommend_from_physical_filaments(
    C3dkmSrgb target_rgb,
    const C3dkmSrgb* filament_colors,
    const unsigned int* filament_indices,
    unsigned int filament_count,
    const char* preferred_material_type,
    C3dRecipeResult* out_result);

CR_CLOSE_API int c3d_recipe_lookup_standard_recipe(
    C3dkmSrgb target_rgb,
    int mode,
    const char* preferred_material_type,
    C3dRecipeResult* out_result);

CR_CLOSE_API int c3d_recipe_has_base_color_spectrum(
    int mode, const char* material, const char* base_color);

/* Set the resources directory used by the recipe engine (overrides the
 * CR_CLOSE_RESOURCES_DIR environment variable). Pass NULL / empty to clear.
 * The path is copied internally. Thread-safe.
 */
CR_CLOSE_API void c3d_recipe_set_resources_dir(const char* path);

/* ===========================================================================
 * Layer 3 - Caller-friendly structured-result wrappers.
 *
 * These are what C3DSlicer's ColorDecomposeRecipe.cpp calls. They are thin
 * adapters over Layer 2: they take std::vector / std::string analogues
 * spelled out as flat C arrays, then call into the engine and translate
 * the result into a single CRCloseRecipeResult that the caller must free
 * via free_close_recipe_result().
 * ======================================================================== */

/* One raw physical filament entry. hex / name / type are NUL-terminated C
 * strings. `spectrum` (43 doubles) may be NULL when no measured spectrum is
 * available; the engine then falls back to a synthetic sRGB spectrum.
 */
struct CRCloseFilament {
    const char* color_hex;       /* "#RRGGBB"                       */
    const char* name;
    const char* type;
    unsigned int filament_index; /* 1-based                         */
    int has_spectrum;            /* 0 / 1                           */
    const double* spectrum;      /* 43 doubles or NULL              */
};

struct CRCloseComponent {
    const char* color_hex;       /* "#RRGGBB" of the recommended     */
    const char* base_color;      /* "Cyan" / "Magenta" / ... or ""   */
    int          ratio;          /* integer 0..100                   */
    unsigned int filament_index; /* 1-based; 0 for standard base     */
    int          has_spectrum;   /* 0 / 1                           */
    const double* spectrum;      /* 43 doubles or NULL              */
};

struct CRCloseRecipeResult {
    int           valid;             /* 0 / 1                       */
    int           mode;              /* 0=MaterialList,1=CMYW,2=RYBW */
    const char*   matched_color_hex; /* "#RRGGBB" or NULL           */
    unsigned int  component_count;
    const CRCloseComponent* components;
};

/* Returns 0 on success, non-zero on error. */
CR_CLOSE_API int cr_close_recommend_from_physical_filaments(
    unsigned char target_r, unsigned char target_g, unsigned char target_b,
    const CRCloseFilament* filaments, unsigned int filament_count,
    const char* preferred_material_type,
    CRCloseRecipeResult* out_result);

CR_CLOSE_API int cr_close_lookup_standard_recipe(
    unsigned char target_r, unsigned char target_g, unsigned char target_b,
    int mode, /* 0=MaterialList, 1=CMYW, 2=RYBW */
    const char* preferred_material_type,
    CRCloseRecipeResult* out_result);

/* 1=CMYW, 2=RYBW. */
CR_CLOSE_API int cr_close_has_base_color_spectrum(
    int mode, const char* material, const char* base_color);

/* Return the official_hex for a base color, read from the
 * base_color_spectra.json resource that ships with the slicer. The
 * returned string points to library-owned memory and is valid for
 * the lifetime of the library. Pass NULL for any parameter to use
 * its default (mode=CMYW, material="Hyper PLA"). Returns a valid
 * "#RRGGBB" string even on error, falling back to the hard-coded
 * table for the requested mode. */
CR_CLOSE_API const char* cr_close_get_base_official_hex(
    int mode, const char* material, const char* base_color);

/* Blend N hex colors via K-M. Result in out_hex (>=8 bytes). */
CR_CLOSE_API int cr_close_km_blend_color_multi_hex(
    const char* const* hex_colors, const int* weights, unsigned int count,
    char* out_hex);

/* Optional override of the resources directory. Pass NULL to clear.
 * The path is copied internally. Thread-safe. */
CR_CLOSE_API void cr_close_set_resources_dir(const char* path);

#ifdef __cplusplus
}
#endif

#endif /* CR_KM_RECIPE_H */
