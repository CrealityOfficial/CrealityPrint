#ifndef GPU_ORIENT_METAL_HPP
#define GPU_ORIENT_METAL_HPP

#include "libslic3r/Orient.hpp"

#include <string>

namespace Slic3r {
namespace orientation {
namespace metal {

bool available(std::string* error = nullptr);
bool orient(OrientMeshs& items, const OrientParams& params, std::string* error);

} // namespace metal
} // namespace orientation
} // namespace Slic3r

#endif // GPU_ORIENT_METAL_HPP
