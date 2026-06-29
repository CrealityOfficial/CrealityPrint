#ifndef slic3r_SlicerBridgeDiagnostics_hpp_
#define slic3r_SlicerBridgeDiagnostics_hpp_

#include "nlohmann/json.hpp"

namespace Slic3r {
namespace GUI {

class Plater;

namespace Bridge {

nlohmann::json BuildCurrentPlateValidationSnapshot(
    Plater* plater,
    bool model_fits,
    bool validate_error);

nlohmann::json BuildStructuredSceneIssuesFromPlateValidation(
    const nlohmann::json& validation);

void AttachCurrentPlateValidationResult(
    nlohmann::json& result,
    Plater* plater,
    bool model_fits,
    bool validate_error,
    const nlohmann::json& params);

} // namespace Bridge
} // namespace GUI
} // namespace Slic3r

#endif // slic3r_SlicerBridgeDiagnostics_hpp_
