#ifndef slic3r_GUI_CxAgentEndpointPolicy_hpp_
#define slic3r_GUI_CxAgentEndpointPolicy_hpp_

#include <string>

namespace Slic3r {
namespace GUI {

enum class CxAgentEndpointEnvironment
{
    Local,
    Development,
    Production
};

enum class CxAgentEndpointSource
{
    LocalOverride,
    ChannelDefault,
    ConfigurationOverride,
    SafeFallback
};

struct CxAgentEndpointSelection
{
    std::string api_base;
    CxAgentEndpointEnvironment environment {CxAgentEndpointEnvironment::Production};
    CxAgentEndpointSource source {CxAgentEndpointSource::SafeFallback};
    bool rejected_configuration {false};
};

// Resolves the shared CxAgent environment policy. Callers keep their own
// module-specific localhost overrides outside this function.
CxAgentEndpointSelection resolve_cxagent_endpoint(
    const std::string& build_channel,
    const std::string& region,
    const std::string& configured_api_base);

bool cxagent_channel_allows_environment_override(const std::string& build_channel);
bool is_cxagent_development_api_base(const std::string& api_base);

const char* cxagent_development_api_base();
const char* cxagent_endpoint_environment_name(CxAgentEndpointEnvironment environment);
const char* cxagent_endpoint_source_name(CxAgentEndpointSource source);

} // namespace GUI
} // namespace Slic3r

#endif // slic3r_GUI_CxAgentEndpointPolicy_hpp_
