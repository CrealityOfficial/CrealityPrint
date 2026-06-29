// [FORMATTED BY CLANG-FORMAT 2026-05-13 13:56:47]
#ifndef slic3r_OrganicSupportValidation_hpp
#define slic3r_OrganicSupportValidation_hpp

#include "SupportLayer.hpp"
#include "TreeSupportCommon.hpp"

#include <functional>

namespace Slic3r {

class PrintObject;
class TreeModelVolumes;

namespace TreeSupport3D {

// Validate and fix floating/hanging supports by growing new support downward.
// Uses a single top-down pass with dual snapshots for correctness:
//   - Floating islands are grown downward through collision-free space.
//   - Islands blocked by model geometry are discarded with a warning.
// Checks and modifies intermediate_layers; bottom_contacts and top_contacts are read-only.
void validate_and_fix_floating_supports(PrintObject&                  print_object,
                                        const TreeModelVolumes&       volumes,
                                        const TreeSupportSettings&    config,
                                        SupportGeneratorLayersPtr&    bottom_contacts,
                                        SupportGeneratorLayersPtr&    top_contacts,
                                        SupportGeneratorLayersPtr&    intermediate_layers,
                                        SupportGeneratorLayerStorage& layer_storage,
                                        std::function<void()>         throw_on_cancel);

} // namespace TreeSupport3D
} // namespace Slic3r

#endif // slic3r_OrganicSupportValidation_hpp
