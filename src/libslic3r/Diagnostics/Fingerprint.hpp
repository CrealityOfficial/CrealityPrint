#ifndef slic3r_Diagnostics_Fingerprint_hpp_
#define slic3r_Diagnostics_Fingerprint_hpp_

#include "../support_new/TreeSupport3D.hpp"

#include <string_view>
#include <vector>

namespace Slic3r {

class PrintObject;
class ConstLayerPtrsAdaptor;
class ConstSupportLayerPtrsAdaptor;
struct SupportNode;

namespace Diagnostics {

struct SupportModuleInput
{
    const PrintObject &object;
};

#if defined(SLIC3R_DIAGNOSTICS_FINGERPRINT_ENABLED)

// Immediately fingerprints data and records it in the PrintObject's Session.
// This is a no-op when fingerprint collection is disabled.
void fingerprint(std::string_view group_path, const PrintObject &object, const SupportModuleInput &input) noexcept;
void fingerprint(std::string_view group_path, const PrintObject &object, const ConstLayerPtrsAdaptor &layers) noexcept;
void fingerprint(std::string_view group_path, const PrintObject &object, const ConstSupportLayerPtrsAdaptor &layers) noexcept;
void fingerprint(std::string_view group_path, const PrintObject &object, const std::vector<std::vector<SupportNode *>> &nodes) noexcept;
void fingerprint(std::string_view group_path, const PrintObject &object, const std::vector<TreeSupport3D::SupportElements> &elements) noexcept;

#else

// Fingerprint instrumentation is compiled out of non-diagnostic builds.
inline void fingerprint(std::string_view, const PrintObject &, const SupportModuleInput &) noexcept
{
}
inline void fingerprint(std::string_view, const PrintObject &, const ConstLayerPtrsAdaptor &) noexcept
{
}
inline void fingerprint(std::string_view, const PrintObject &, const ConstSupportLayerPtrsAdaptor &) noexcept
{
}
inline void fingerprint(std::string_view, const PrintObject &, const std::vector<std::vector<SupportNode *>> &) noexcept
{
}
inline void fingerprint(std::string_view, const PrintObject &, const std::vector<TreeSupport3D::SupportElements> &) noexcept
{
}
#endif

} // namespace Diagnostics
} // namespace Slic3r

#endif
