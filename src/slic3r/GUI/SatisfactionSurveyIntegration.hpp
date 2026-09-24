#ifndef slic3r_GUI_SatisfactionSurveyIntegration_hpp_
#define slic3r_GUI_SatisfactionSurveyIntegration_hpp_

#include <string>

namespace Slic3r {
namespace GUI {

// Explicit registration keeps the passive WebView observer available in every
// build configuration without relying on wxModule static initialization.
void install_satisfaction_survey_event_filter();
void uninstall_satisfaction_survey_event_filter();

// AI print completion is a native callback rather than a WebView command, so it
// uses this single thin adapter.  Other print paths are observed through their
// existing WebView lifecycle messages by SatisfactionSurveyIntegration.cpp.
void record_ai_satisfaction_survey_print(const std::string& device_address,
                                         const std::string& operation_id);

// Multi-device printing opens the device page before any upload has finished.
// Pair that early request with the existing per-device upload terminal callbacks
// so cancelled/failed transfers are not counted as successful print starts.
void notify_satisfaction_survey_multi_device_upload_result(const std::string& device_address,
                                                           bool               succeeded);

} // namespace GUI
} // namespace Slic3r

#endif // slic3r_GUI_SatisfactionSurveyIntegration_hpp_
