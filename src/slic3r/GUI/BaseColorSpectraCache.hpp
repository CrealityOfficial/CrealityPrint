#pragma once

#include <nlohmann/json_fwd.hpp>

namespace Slic3r { namespace GUI { namespace BaseColorSpectraCache {

// Seed the writable cache from the bundled resource when necessary and point
// the K-M recipe engine at it. A valid network-updated cache is preserved.
bool initialize();

// Rebuild the eight CMYW/RYBW Hyper PLA spectra from official/materialList's
// normalized color cache. On any validation or I/O failure, keeps the previous
// base_color_spectra.json unchanged.
bool update_from_material_cache(const nlohmann::json& material_cache);

}}} // namespace Slic3r::GUI::BaseColorSpectraCache
