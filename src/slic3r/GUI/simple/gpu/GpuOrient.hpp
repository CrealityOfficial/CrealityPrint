#ifndef GPU_ORIENT_HPP
#define GPU_ORIENT_HPP

#include "libslic3r/Orient.hpp"

#include <memory>
#include <string>

namespace Slic3r {
namespace orientation {

// GPU-accelerated orienter supporting MinArea, MinVolume and MinTime modes.
// Uses OpenGL 4.3 compute shaders (Windows: WGL context, Linux: EGL headless).
// Falls back to CPU automatically when GPU is unavailable or init fails.
class GpuOrient {
public:
    GpuOrient();
    ~GpuOrient();

    // Whether the GPU path is ready (GL context + compute shader compiled).
    bool available() const noexcept;

    // Try to orient meshes on GPU. If fallback_to_cpu is true and GPU is
    // unavailable or fails, it will invoke the existing CPU orient().
    bool orient(OrientMeshs &items,
                const OrientMeshs &excludes,
                const OrientParams &params,
                bool fallback_to_cpu = true,
                std::string *error = nullptr);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace orientation
} // namespace Slic3r

#endif // GPU_ORIENT_HPP
