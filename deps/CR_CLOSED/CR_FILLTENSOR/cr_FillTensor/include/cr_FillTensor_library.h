#ifndef CR_FILLTENSOR_LIBRARY_H
#define CR_FILLTENSOR_LIBRARY_H

#if defined(CR_FILLTENSOR_STATIC_DEFINE)
    #define CR_FILLTENSOR_API
#elif defined(_WIN32)
    #ifdef CR_FILLTENSOR_LIBRARY_EXPORTS
        #define CR_FILLTENSOR_API __declspec(dllexport)
    #else
        #define CR_FILLTENSOR_API __declspec(dllimport)
    #endif
#else
    #define CR_FILLTENSOR_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Opaque handle
typedef void* FillTensorHandle;

// ============================================================
// Minimal C API: five functions.
// ============================================================

/// Create a tensor-fill instance.
/// cell_type follows the slicer enum: 0=Schwarz-D, 1=Gyroid, 2=Fischer-Koch.
/// Returns NULL on failure.
CR_FILLTENSOR_API FillTensorHandle FillTensor_Create(int cell_type);

/// Destroy an instance.
CR_FILLTENSOR_API void FillTensor_Destroy(FillTensorHandle handle);

/// Set algorithm parameters before FillTensor_SetFieldAndCompute.
/// surface_density: surface density in [1, 99] (%).
/// interior_density: interior density in [1, 99] (%).
/// scale_w_factor: global frequency factor, default 3.0.
CR_FILLTENSOR_API void FillTensor_SetParams(FillTensorHandle handle,
                                          float surface_density,
                                          float interior_density,
                                          float scale_w_factor);

/// Set the 3D SDF grid and compute the tensor-driven field.
/// sdf_grid: flat array stored in [i][j][k] order, size = nx * ny * nz.
///           Values < 0 are inside the model; values > 0 are outside.
/// nx, ny, nz: grid dimensions.
/// gap: grid spacing in mm.
/// bbox_min_x/y/z: world-space coordinate of the grid origin in mm.
/// Returns 0 on success, nonzero on failure.
CR_FILLTENSOR_API int FillTensor_SetFieldAndCompute(FillTensorHandle handle,
                                                  const float* sdf_grid,
                                                  int nx, int ny, int nz,
                                                  float gap,
                                                  float bbox_min_x, float bbox_min_y, float bbox_min_z);

/// Query one scalar-field value.
/// x, y, z: world-space coordinates in mm.
/// Return value < 0 means solid lattice wall; > 0 means lattice void.
CR_FILLTENSOR_API float FillTensor_Query(FillTensorHandle handle, float x, float y, float z);

/// Batch-query scalar-field values for per-layer infill generation.
/// points: [x0,y0,z0, x1,y1,z1, ...], size = count * 3.
/// results: preallocated output array, size = count.
/// count: number of query points.
CR_FILLTENSOR_API void FillTensor_QueryBatch(FillTensorHandle handle,
                                           const float* points,
                                           float* results,
                                           int count);

#ifdef __cplusplus
}
#endif

#endif // CR_FILLTENSOR_LIBRARY_H
