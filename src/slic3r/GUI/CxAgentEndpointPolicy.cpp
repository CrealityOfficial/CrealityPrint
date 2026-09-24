#include "CxAgentEndpointPolicy.hpp"

#include <algorithm>
#include <cctype>

namespace Slic3r {
namespace GUI {
namespace {

constexpr char kCxAgentDevelopmentApiBase[] = "https://cxagent-dev.crealitycloud.cn";
constexpr char kCxAgentChinaProductionApiBase[] = "https://cxagent.crealitycloud.cn";
constexpr char kCxAgentGlobalProductionApiBase[] = "https://cxagent.crealitycloud.com";

std::string normalized_token(std::string value)
{
    const auto is_space = [](unsigned char ch) { return std::isspace(ch) != 0; };
    value.erase(value.begin(), std::find_if_not(value.begin(), value.end(), is_space));
    value.erase(std::find_if_not(value.rbegin(), value.rend(), is_space).base(), value.end());
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return value;
}

std::string normalized_known_api_base(std::string value)
{
    const auto is_space = [](unsigned char ch) { return std::isspace(ch) != 0; };
    value.erase(value.begin(), std::find_if_not(value.begin(), value.end(), is_space));
    value.erase(std::find_if_not(value.rbegin(), value.rend(), is_space).base(), value.end());
    while (!value.empty() && value.back() == '/')
        value.pop_back();

    if (value == kCxAgentDevelopmentApiBase ||
        value == kCxAgentChinaProductionApiBase ||
        value == kCxAgentGlobalProductionApiBase)
        return value;
    return {};
}

std::string production_api_base(const std::string& region)
{
    return normalized_token(region) == "china"
        ? kCxAgentChinaProductionApiBase
        : kCxAgentGlobalProductionApiBase;
}

} // namespace

CxAgentEndpointSelection resolve_cxagent_endpoint(
    const std::string& build_channel,
    const std::string& region,
    const std::string& configured_api_base)
{
    const std::string channel = normalized_token(build_channel);
    if (channel == "alpha") {
        return {kCxAgentDevelopmentApiBase,
                CxAgentEndpointEnvironment::Development,
                CxAgentEndpointSource::ChannelDefault,
                false};
    }

    const std::string production_base = production_api_base(region);
    if (channel == "beta") {
        if (configured_api_base.empty()) {
            return {production_base,
                    CxAgentEndpointEnvironment::Production,
                    CxAgentEndpointSource::ChannelDefault,
                    false};
        }

        const std::string configured_base = normalized_known_api_base(configured_api_base);
        if (!configured_base.empty()) {
            return {configured_base,
                    configured_base == kCxAgentDevelopmentApiBase
                        ? CxAgentEndpointEnvironment::Development
                        : CxAgentEndpointEnvironment::Production,
                    CxAgentEndpointSource::ConfigurationOverride,
                    false};
        }

        return {production_base,
                CxAgentEndpointEnvironment::Production,
                CxAgentEndpointSource::SafeFallback,
                true};
    }

    // Release and unknown channels fail safely to the regional production
    // endpoint and never honor a user-editable configuration override.
    return {production_base,
            CxAgentEndpointEnvironment::Production,
            channel == "release"
                ? CxAgentEndpointSource::ChannelDefault
                : CxAgentEndpointSource::SafeFallback,
            false};
}

bool cxagent_channel_allows_environment_override(const std::string& build_channel)
{
    return normalized_token(build_channel) == "beta";
}

bool is_cxagent_development_api_base(const std::string& api_base)
{
    return normalized_known_api_base(api_base) == kCxAgentDevelopmentApiBase;
}

const char* cxagent_development_api_base()
{
    return kCxAgentDevelopmentApiBase;
}

const char* cxagent_endpoint_environment_name(CxAgentEndpointEnvironment environment)
{
    switch (environment) {
    case CxAgentEndpointEnvironment::Local:
        return "local";
    case CxAgentEndpointEnvironment::Development:
        return "development";
    case CxAgentEndpointEnvironment::Production:
        return "production";
    }
    return "unknown";
}

const char* cxagent_endpoint_source_name(CxAgentEndpointSource source)
{
    switch (source) {
    case CxAgentEndpointSource::LocalOverride:
        return "local_override";
    case CxAgentEndpointSource::ChannelDefault:
        return "channel_default";
    case CxAgentEndpointSource::ConfigurationOverride:
        return "configuration_override";
    case CxAgentEndpointSource::SafeFallback:
        return "safe_fallback";
    }
    return "unknown";
}

} // namespace GUI
} // namespace Slic3r
