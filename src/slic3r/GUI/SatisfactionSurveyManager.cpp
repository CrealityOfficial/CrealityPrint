#include "SatisfactionSurveyManager.hpp"

#include "CxAgentEndpointPolicy.hpp"
#include "GUI_App.hpp"
#include "Plater.hpp"
#include "SatisfactionSurveyDialog.hpp"
#include "Widgets/WebView.hpp"
#include "print_manage/data/DataCenter.hpp"

#include "libslic3r/AppConfig.hpp"
#include "libslic3r/Time.hpp"
#include "libslic3r/common_header/common_header.h"
#include "slic3r/Utils/Http.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <chrono>
#include <cctype>
#include <ctime>
#include <limits>
#include <map>
#include <sstream>
#include <utility>
#include <vector>

#include <boost/log/trivial.hpp>

#include <wx/timer.h>
#include <wx/toplevel.h>
#include <wx/utils.h>
#ifdef _WIN32
#include <wx/webrequest.h>
#endif
#include <wx/weakref.h>
#include <wx/window.h>

namespace Slic3r {
namespace GUI {

// For local service debugging, define SATISFACTION_SURVEY_LOCAL_DEBUG in an
// uncommitted build.  Shared builds use the regional production server.
// #define SATISFACTION_SURVEY_LOCAL_DEBUG

namespace {

constexpr int kRequiredSuccessfulPrints = 3;
constexpr std::time_t kRequiredUsageSeconds = 7 * 24 * 60 * 60;
constexpr int kConfigConnectTimeoutSeconds = 2;
constexpr int kConfigRequestTimeoutSeconds = 5;
constexpr int kStartupWindowMilliseconds = 45000;
constexpr int kStartupPollMilliseconds = 200;
constexpr int kQuietWindowMilliseconds = 2000;
constexpr int kInactiveGraceMilliseconds = 1000;
constexpr int kDuplicatePrintWindowMilliseconds = 1000;
constexpr std::size_t kConfigResponseLimitBytes = 64 * 1024;
constexpr std::size_t kMaxPrinterModels = 32;
constexpr int kMaxConnectedPrinters = 1000;

// Device-page device types.  Only 0 (LAN) and 1 (Creality Cloud) are Creality
// printers; every other value (1001 = fluidd, plus any future third-party
// entry) is shown as "Other" on the device page.
constexpr int kLocalDeviceType = 0;
constexpr int kCloudDeviceType = 1;
constexpr char kCrealityBrandCode[] = "creality";
constexpr char kOtherBrandCode[] = "other";
constexpr char kOtherModelCode[] = "other";
constexpr char kOtherModelName[] = "Other";
// Device identity keys are namespaced so that a third-party entry can never be
// merged into the Creality pool (or the other way round) when both happen to
// expose the same MAC.
constexpr char kCrealityIdentityPrefix[] = "mac:";
constexpr char kOtherIdentityPrefix[] = "type:other|";
constexpr char kTlsDebugLogPrefix[] = "[SAT_SURVEY_TLS_DEBUG][SatisfactionSurvey]";
constexpr char kSurveyStateSection[] = "satisfaction_survey";
constexpr char kLegacySurveyStateSectionPrefix[] = "satisfaction_survey.";
constexpr char kLegacySurveyStateSectionIndexKey[] = "satisfaction_survey_state_section";

enum class RemoteState
{
    Unknown,
    Enabled,
    Disabled
};

bool parse_bool(const std::string& value)
{
    std::string normalized = value;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return normalized == "1" || normalized == "true" || normalized == "yes";
}

long long parse_non_negative_integer(const std::string& value, long long fallback)
{
    try {
        std::size_t consumed = 0;
        const long long parsed = std::stoll(value, &consumed);
        if (consumed == value.size() && parsed >= 0)
            return parsed;
    } catch (const std::exception&) {
    }
    return fallback;
}

std::string format_config_time(std::time_t value)
{
    return Utils::utc_timestamp(value);
}

std::time_t parse_config_time(const std::string& value)
{
    const std::time_t parsed = Utils::str2time(value, Utils::TimeZone::utc, Utils::TimeFormat::gcode);
    if (parsed == std::time_t(-1) || format_config_time(parsed) != value)
        return std::time_t(-1);
    return parsed;
}

std::time_t parse_legacy_epoch_time(const std::string& value)
{
    const long long parsed = parse_non_negative_integer(value, -1);
    if (parsed <= 0)
        return std::time_t(-1);

    const std::time_t converted = static_cast<std::time_t>(parsed);
    return static_cast<long long>(converted) == parsed ? converted : std::time_t(-1);
}

CxAgentEndpointSelection survey_endpoint_selection(const AppConfig& app_config,
                                                   const std::string& channel)
{
#ifdef SATISFACTION_SURVEY_LOCAL_DEBUG
    (void) app_config;
    (void) channel;
    return {"http://127.0.0.1:8787",
            CxAgentEndpointEnvironment::Local,
            CxAgentEndpointSource::LocalOverride,
            false};
#else
    return resolve_cxagent_endpoint(
        channel,
        app_config.get("region"),
        app_config.get("cxagent_api_base"));
#endif
}

const char* survey_platform_name()
{
#ifdef _WIN32
    return "windows";
#elif defined(__APPLE__)
    return "macos";
#else
    return "linux";
#endif
}

std::string normalized_mac(const std::string& mac)
{
    std::string normalized;
    normalized.reserve(mac.size());
    for (const unsigned char ch : mac) {
        if (ch == ':' || ch == '-' || ch == '.' || std::isspace(ch))
            continue;
        if (!std::isxdigit(ch))
            return {};
        normalized.push_back(static_cast<char>(std::tolower(ch)));
    }

    // A printer MAC is 6 bytes. Reject placeholders and incomplete values to
    // avoid merging unrelated devices under the same invalid identity.
    return normalized.size() == 12 ? normalized : std::string {};
}

std::string normalized_text_identity(std::string value)
{
    const auto first = std::find_if_not(value.begin(), value.end(), [](unsigned char ch) { return std::isspace(ch); });
    const auto last  = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char ch) { return std::isspace(ch); }).base();
    if (first >= last)
        return {};

    value = std::string(first, last);
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

std::string json_string_or_empty(const nlohmann::json& value, const char* key)
{
    const auto iter = value.find(key);
    return iter != value.end() && iter->is_string() ? iter->get<std::string>() : std::string {};
}

struct SurveyDeviceSnapshot
{
    DM::Device device;
    std::string identity;
    // False when the entry carries no usable MAC: third-party printers store
    // their host URL in "mac", so such a record can only be counted as its own
    // machine and is never merged with another record.
    bool has_valid_mac = false;
};

bool is_other_survey_device(const DM::Device& device)
{
    return device.deviceType != kLocalDeviceType && device.deviceType != kCloudDeviceType;
}

std::string survey_device_identity(const nlohmann::json& printer, const DM::Device& device)
{
    if (const std::string mac = normalized_mac(device.mac); !mac.empty())
        return (is_other_survey_device(device) ? kOtherIdentityPrefix : kCrealityIdentityPrefix) + mac;

    // The device page links LAN and cloud entries by storing their peer
    // addresses in identity. Sorting the pair makes both entries produce the
    // same key without changing the shared device model.
    std::string address = normalized_text_identity(device.address);
    std::string peer_address = normalized_text_identity(json_string_or_empty(printer, "identity"));
    if (!address.empty() && !peer_address.empty()) {
        if (peer_address < address)
            std::swap(address, peer_address);
        return "linked:" + address + "|" + peer_address;
    }

    if (const std::string tb_id = normalized_text_identity(device.tbId); !tb_id.empty())
        return "tbid:" + tb_id;

    return address.empty() ? std::string {} : "address:" + address;
}

std::vector<SurveyDeviceSnapshot> survey_device_page_snapshot(const nlohmann::json& data)
{
    std::vector<SurveyDeviceSnapshot> devices;
    if (!data.is_object() || !data.contains("data") || !data["data"].is_object())
        return devices;

    const auto& payload = data["data"];
    if (!payload.contains("printerList") || !payload["printerList"].is_array())
        return devices;

    for (const auto& group : payload["printerList"]) {
        if (!group.is_object() || !group.contains("list") || !group["list"].is_array())
            continue;

        for (const auto& printer : group["list"]) {
            if (!printer.is_object())
                continue;

            const auto type_iter = printer.find("deviceType");
            if (type_iter == printer.end() || !type_iter->is_number_integer())
                continue;

            const int device_type = type_iter->get<int>();
            if (device_type < 0)
                continue;

            nlohmann::json printer_copy = printer;
            DM::Device device = DM::Device::deserialize(printer_copy, false);
            if (!device.valid)
                continue;

            const std::string identity = survey_device_identity(printer, device);
            if (identity.empty())
                continue;

            SurveyDeviceSnapshot snapshot;
            snapshot.identity = identity;
            snapshot.has_valid_mac = !normalized_mac(device.mac).empty();
            snapshot.device = std::move(device);
            devices.push_back(std::move(snapshot));
        }
    }
    return devices;
}

void supplement_survey_device(DM::Device& target, const DM::Device& source)
{
    if (target.mac.empty())
        target.mac = source.mac;
    if (target.address.empty())
        target.address = source.address;
    if (target.tbId.empty())
        target.tbId = source.tbId;
    if (target.model.empty())
        target.model = source.model;
    if (target.modelName.empty())
        target.modelName = source.modelName;
    if (target.name.empty())
        target.name = source.name;
    target.online = target.online || source.online;
}

void merge_survey_device(DM::Device& target, const DM::Device& source)
{
    // Prefer the LAN entry because it carries the fields shown on the device
    // page, then supplement fields that are available only from cloud data.
    if (target.deviceType != 0 && source.deviceType == 0) {
        DM::Device previous = target;
        target = source;
        supplement_survey_device(target, previous);
    } else {
        supplement_survey_device(target, source);
    }
}

std::map<std::string, DM::Device> deduplicate_survey_devices(const std::vector<SurveyDeviceSnapshot>& devices,
                                                            int& undeduplicated_other_count)
{
    std::map<std::string, DM::Device> unique_devices;
    undeduplicated_other_count = 0;
    for (const SurveyDeviceSnapshot& snapshot : devices) {
        // Third-party entries without a usable MAC cannot be proven identical to
        // any other record, so every one of them is counted separately.
        if (is_other_survey_device(snapshot.device) && !snapshot.has_valid_mac) {
            ++undeduplicated_other_count;
            continue;
        }

        const auto [iter, inserted] = unique_devices.emplace(snapshot.identity, snapshot.device);
        if (!inserted)
            merge_survey_device(iter->second, snapshot.device);
    }
    return unique_devices;
}

struct SurveyPrinterSummary
{
    // Creality models in the order of model code / model name, with quantity.
    std::map<std::pair<std::string, std::string>, int> creality_models;
    // Imported third-party printers.  They are reported as one "Other" entry,
    // so only the machine count is kept.
    int other_count = 0;
};

SurveyPrinterSummary summarize_survey_printers(const std::map<std::string, DM::Device>& unique_devices,
                                               int      undeduplicated_other_count)
{
    SurveyPrinterSummary summary;
    summary.other_count = undeduplicated_other_count;

    for (const auto& item : unique_devices) {
        const DM::Device& device = item.second;
        // Imported third-party printers are not Creality models: they are
        // reported as a single "Other" entry and only their quantity matters.
        if (is_other_survey_device(device)) {
            ++summary.other_count;
            continue;
        }

        const std::string model_name = device.modelName.empty() ? device.model : device.modelName;
        if (model_name.empty())
            continue;
        ++summary.creality_models[{device.model, model_name}];
    }
    return summary;
}

nlohmann::json connected_creality_printers()
{
    std::map<std::string, DM::Device> unique_devices;
    int undeduplicated_other_count = 0;
    try {
        const nlohmann::json data = DM::DataCenter::Ins().GetData();
        unique_devices = deduplicate_survey_devices(survey_device_page_snapshot(data),
                                                    undeduplicated_other_count);
    } catch (const std::exception& exception) {
        BOOST_LOG_TRIVIAL(warning)
            << "[SatisfactionSurvey] failed to snapshot device-page printer models: "
            << exception.what();
    }

    const SurveyPrinterSummary summary =
        summarize_survey_printers(unique_devices, undeduplicated_other_count);

    nlohmann::json result = {
        {"collection_scope", "creality_only"},
        {"connected_count", 0},
        {"printers", nlohmann::json::array()}
    };
    int connected_count = 0;
    std::size_t model_count = 0;
    for (const auto& item : summary.creality_models) {
        if (model_count >= kMaxPrinterModels || connected_count >= kMaxConnectedPrinters)
            break;
        const int quantity = std::min(item.second, kMaxConnectedPrinters - connected_count);
        result["printers"].push_back({
            {"brand_code", kCrealityBrandCode},
            {"model_code", item.first.first},
            {"model_name", item.first.second},
            {"quantity", quantity}
        });
        connected_count += quantity;
        ++model_count;
    }
    if (summary.other_count > 0 && model_count < kMaxPrinterModels &&
        connected_count < kMaxConnectedPrinters) {
        const int quantity =
            std::min(summary.other_count, kMaxConnectedPrinters - connected_count);
        result["printers"].push_back({
            {"brand_code", kOtherBrandCode},
            {"model_code", kOtherModelCode},
            {"model_name", kOtherModelName},
            {"quantity", quantity}
        });
        connected_count += quantity;
    }
    result["connected_count"] = connected_count;
    return result;
}

nlohmann::json survey_shown_context(const AppConfig& app_config,
                                    const std::string& full_version,
                                    const std::string& channel)
{
    const std::string locale = app_config.get("language").empty()
        ? std::string("en_GB")
        : app_config.get("language");
    const UserInfo& user = wxGetApp().get_user();

    nlohmann::json user_context;
    user_context["user_id"] = user.bLogin && !user.userId.empty()
        ? nlohmann::json(user.userId)
        : nlohmann::json(nullptr);

    return {
        {"client", {
            {"product", "creality-print"},
            {"app_version", full_version},
            {"build_version", CxBuildInfo::getBuildId()},
            {"channel", channel},
            {"platform", survey_platform_name()},
            {"os_version", wxGetOsDescription().ToUTF8().data()},
            {"locale", locale},
            {"region", app_config.get("region")}
        }},
        {"user", std::move(user_context)},
        {"devices", connected_creality_printers()}
    };
}

std::string append_query(std::string url, const std::string& query)
{
    const std::size_t fragment = url.find('#');
    const std::size_t end = fragment == std::string::npos ? url.size() : fragment;
    const char separator = url.substr(0, end).find('?') == std::string::npos ? '?' : '&';
    url.insert(end, std::string(1, separator) + query);
    return url;
}

const char* print_source_name(SatisfactionSurveyPrintSource source)
{
    switch (source) {
    case SatisfactionSurveyPrintSource::SendToPrinter: return "send";
    case SatisfactionSurveyPrintSource::DeviceDetail: return "detail";
    case SatisfactionSurveyPrintSource::MultiDevice: return "multi";
    case SatisfactionSurveyPrintSource::AiAssistant: return "ai";
    }
    return "unknown";
}

#ifdef _WIN32
std::string sanitized_web_request_error(std::string result)
{
    std::replace_if(result.begin(), result.end(),
                    [](unsigned char ch) { return ch < 0x20 || ch == 0x7f; }, ' ');

    // Keep transport diagnostics useful without writing URL query parameters
    // to the application log if wx ever includes the request URL in an error.
    std::size_t search_from = 0;
    while (true) {
        const std::size_t scheme = result.find("://", search_from);
        if (scheme == std::string::npos)
            break;
        const std::size_t url_end = result.find(' ', scheme + 3);
        const std::size_t query = result.find_first_of("?#", scheme + 3);
        if (query != std::string::npos &&
            (url_end == std::string::npos || query < url_end)) {
            const std::size_t erase_end = url_end == std::string::npos ? result.size() : url_end;
            result.replace(query, erase_end - query, "?<redacted>");
            search_from = query + sizeof("?<redacted>") - 1;
        } else {
            search_from = url_end == std::string::npos ? result.size() : url_end + 1;
        }
    }

    constexpr std::size_t kMaxDiagnosticLength = 256;
    if (result.size() > kMaxDiagnosticLength)
        result.replace(kMaxDiagnosticLength, std::string::npos, "...");
    return result;
}

std::string sanitized_web_request_error(const wxString& value)
{
    const wxScopedCharBuffer utf8 = value.ToUTF8();
    return sanitized_web_request_error(
        std::string(utf8.data() != nullptr ? utf8.data() : ""));
}
#endif

bool has_other_modal_dialog(const wxWindow* survey_dialog)
{
    for (wxWindow* window : wxTopLevelWindows) {
        if (window == nullptr || window == survey_dialog || !window->IsShown())
            continue;
        const auto* dialog = dynamic_cast<const wxDialog*>(window);
        if (dialog != nullptr && dialog->IsModal())
            return true;
    }
    return false;
}

} // namespace

class SatisfactionSurveyManager::Impl final : public wxEvtHandler
{
public:
    Impl(AppConfig& app_config, std::string full_version, std::string channel)
        : m_app_config(app_config)
        , m_full_version(std::move(full_version))
        , m_channel(std::move(channel))
        , m_state_section(kSurveyStateSection)
        , m_endpoint_selection(survey_endpoint_selection(m_app_config, m_channel))
        , m_survey_origin(m_endpoint_selection.api_base)
        , m_timer(this)
#ifdef _WIN32
        , m_config_timeout_timer(this)
#endif
    {
        Bind(wxEVT_TIMER, &Impl::on_timer, this, m_timer.GetId());
#ifdef _WIN32
        Bind(wxEVT_TIMER, &Impl::on_config_request_timeout, this,
             m_config_timeout_timer.GetId());
        Bind(wxEVT_WEBREQUEST_DATA, &Impl::on_config_request_data, this);
        Bind(wxEVT_WEBREQUEST_STATE, &Impl::on_config_request_state, this);
#endif
    }

    ~Impl() override
    {
        shutdown();
#ifdef _WIN32
        Unbind(wxEVT_WEBREQUEST_STATE, &Impl::on_config_request_state, this);
        Unbind(wxEVT_WEBREQUEST_DATA, &Impl::on_config_request_data, this);
        Unbind(wxEVT_TIMER, &Impl::on_config_request_timeout, this,
               m_config_timeout_timer.GetId());
#endif
        Unbind(wxEVT_TIMER, &Impl::on_timer, this, m_timer.GetId());
    }

    void begin_startup_check()
    {
        if (m_check_started || m_shutting_down || m_startup_window_closed)
            return;
        initialize_launch_state();
        m_check_started = true;

        if (!m_eligible_this_launch) {
            m_startup_window_closed = true;
            return;
        }

        BOOST_LOG_TRIVIAL(warning)
            << "[SatisfactionSurvey] endpoint_policy"
            << " channel=" << m_channel
            << " environment="
            << cxagent_endpoint_environment_name(m_endpoint_selection.environment)
            << " source=" << cxagent_endpoint_source_name(m_endpoint_selection.source)
            << " rejected_configuration="
            << (m_endpoint_selection.rejected_configuration ? "true" : "false")
            << " origin=" << m_survey_origin;

        const std::string locale = m_app_config.get("language").empty()
            ? std::string("en_GB")
            : m_app_config.get("language");
        const std::string region = m_app_config.get("region");
        std::ostringstream query;
        query << "product=creality-print"
              << "&app_version=" << Http::url_encode(m_full_version)
              << "&channel=" << Http::url_encode(m_channel)
              << "&platform=" << survey_platform_name()
              << "&locale=" << Http::url_encode(locale)
              << "&region=" << Http::url_encode(region);

        const std::string url = m_survey_origin +
            "/api/v1/surveys/software-satisfaction/config?" + query.str();
#ifdef _WIN32
        start_windows_config_request(url);
#else
        std::weak_ptr<int> lifetime = m_async_lifetime;

        try {
            Http request = Http::get(url);
            request.clear_header()
                .header("Accept", "application/json")
                .ssl_verify_peer(true)
                .ssl_verify_host(true)
                .timeout_connect(kConfigConnectTimeoutSeconds)
                .timeout_max(kConfigRequestTimeoutSeconds)
                .size_limit(kConfigResponseLimitBytes)
                .on_complete([this, lifetime](std::string body, unsigned status) {
                    if (lifetime.expired() || wxTheApp == nullptr)
                        return;
                    wxTheApp->CallAfter([this, lifetime, body = std::move(body), status]() mutable {
                        if (lifetime.expired())
                            return;
                        handle_config_response(std::move(body), status);
                    });
                })
                .on_error([this, lifetime](std::string, std::string error, unsigned status) {
                    if (lifetime.expired() || wxTheApp == nullptr)
                        return;
                    wxTheApp->CallAfter([this, lifetime, error = std::move(error), status]() {
                        if (lifetime.expired())
                            return;
                        BOOST_LOG_TRIVIAL(warning)
                            << "[SatisfactionSurvey] remote config unavailable, status=" << status
                            << ", error=" << error;
                        finish_without_showing("remote config request failed");
                    });
                });
            m_config_request = request.perform();
        } catch (const std::exception& exception) {
            BOOST_LOG_TRIVIAL(warning) << "[SatisfactionSurvey] remote config could not start: "
                                       << exception.what();
            finish_without_showing("remote config request could not start");
        }
#endif
    }

    void expect_startup_restore()
    {
        if (!m_post_init_finished && !m_startup_window_closed)
            m_waiting_for_startup_restore = true;
    }

    void notify_startup_restore_finished()
    {
        if (!m_waiting_for_startup_restore)
            return;
        m_waiting_for_startup_restore = false;
        m_startup_tail_released = true;
        m_quiet_since = {};
    }

    void notify_post_init_finished(wxWindow* parent)
    {
        if (m_post_init_finished || m_shutting_down || m_startup_window_closed)
            return;
        m_post_init_finished = true;
        m_parent = parent;
        m_post_init_time = std::chrono::steady_clock::now();
        m_deadline = m_post_init_time + std::chrono::milliseconds(kStartupWindowMilliseconds);
        if (!m_waiting_for_startup_restore)
            m_startup_tail_released = true;

        ensure_dialog_loading();
        m_timer.Start(kStartupPollMilliseconds);
    }

    void record_successful_print(const SatisfactionSurveyPrintEvent& event)
    {
        if (m_shutting_down)
            return;
        if (!wxIsMainThread()) {
            BOOST_LOG_TRIVIAL(error) << "[SatisfactionSurvey] print event ignored off the UI thread";
            return;
        }
        initialize_launch_state();
        if (m_consumed) {
            debug_print_decision(event.source, "ignored_already_consumed");
            return;
        }

        std::vector<std::string> own_device_addresses;
        own_device_addresses.reserve(event.device_addresses.size());
        for (const std::string& address : event.device_addresses) {
            if (address.empty())
                continue;
            const DM::Device device = DM::DataCenter::Ins().get_printer_data(address);
            if (device.valid && (device.deviceType == 0 || device.deviceType == 1))
                own_device_addresses.emplace_back(address);
        }
        if (own_device_addresses.empty()) {
            debug_print_decision(event.source, "ignored_no_creality_device");
            return;
        }

        std::sort(own_device_addresses.begin(), own_device_addresses.end());
        own_device_addresses.erase(
            std::unique(own_device_addresses.begin(), own_device_addresses.end()),
            own_device_addresses.end());

        std::ostringstream fingerprint;
        fingerprint << print_source_name(event.source) << '|';
        if (!event.operation_fingerprint.empty()) {
            fingerprint << event.operation_fingerprint;
        } else {
            for (const std::string& address : own_device_addresses)
                fingerprint << address << ';';
        }

        const auto now = std::chrono::steady_clock::now();
        const std::string key = fingerprint.str();
        const auto duplicate = m_recent_prints.find(key);
        if (duplicate != m_recent_prints.end() &&
            now - duplicate->second < std::chrono::milliseconds(kDuplicatePrintWindowMilliseconds)) {
            debug_print_decision(event.source, "ignored_duplicate");
            return;
        }
        m_recent_prints[key] = now;
        for (auto it = m_recent_prints.begin(); it != m_recent_prints.end();) {
            if (now - it->second > std::chrono::seconds(10))
                it = m_recent_prints.erase(it);
            else
                ++it;
        }

        if (m_post_init_finished && !m_startup_window_closed)
            m_user_activity_during_startup = true;

        if (m_successful_print_count == std::numeric_limits<long long>::max()) {
            debug_print_decision(event.source, "ignored_counter_saturated");
            return;
        }

        ++m_successful_print_count;
        m_app_config.set(m_state_section, "successful_print_count",
                         std::to_string(m_successful_print_count));
        save_config("successful print count");
        debug_print_decision(event.source, "accepted");
        BOOST_LOG_TRIVIAL(info) << "[SatisfactionSurvey] successful Creality network print recorded, count="
                                << m_successful_print_count;
    }

    void shutdown()
    {
        if (m_shutting_down)
            return;
        m_shutting_down = true;
        m_startup_window_closed = true;
        m_timer.Stop();
#ifdef _WIN32
        m_config_timeout_timer.Stop();
#endif
        m_async_lifetime.reset();
        cancel_config_request("shutdown");
        if (SatisfactionSurveyDialog* active_dialog = m_active_dialog.get()) {
            if (active_dialog->IsModal())
                active_dialog->EndModal(wxID_ABORT);
            else if (!active_dialog->IsBeingDeleted())
                active_dialog->Destroy();
        }
        m_active_dialog = nullptr;
        destroy_dialog();
    }

private:
#ifdef _WIN32
    void start_windows_config_request(const std::string& url)
    {
        try {
            if (!wxWebSession::IsBackendAvailable(wxWebSessionBackendWinHTTP)) {
                BOOST_LOG_TRIVIAL(warning)
                    << kTlsDebugLogPrefix << " config_request"
                    << " result=winhttp_backend_unavailable";
                finish_without_showing("WinHTTP backend is unavailable");
                return;
            }

            if (!m_config_session.IsOpened())
                m_config_session = wxWebSession::New(wxWebSessionBackendWinHTTP);
            if (!m_config_session.IsOpened()) {
                BOOST_LOG_TRIVIAL(warning)
                    << kTlsDebugLogPrefix << " config_request"
                    << " result=winhttp_session_unavailable";
                finish_without_showing("WinHTTP session could not be created");
                return;
            }

            m_config_response_body.clear();
            m_config_response_too_large = false;
            m_config_request = m_config_session.CreateRequest(
                this, wxString::FromUTF8(url.c_str()));
            if (!m_config_request.IsOk()) {
                BOOST_LOG_TRIVIAL(warning)
                    << kTlsDebugLogPrefix << " config_request"
                    << " result=request_creation_failed";
                finish_without_showing("WinHTTP request could not be created");
                return;
            }

            m_config_request.SetHeader("Accept", "application/json");
            m_config_request.SetStorage(wxWebRequest::Storage_None);
            m_config_request.SetTimeouts(kConfigConnectTimeoutSeconds * 1000L,
                                         kConfigRequestTimeoutSeconds * 1000L);
            if (!m_config_timeout_timer.StartOnce(kConfigRequestTimeoutSeconds * 1000)) {
                BOOST_LOG_TRIVIAL(warning)
                    << kTlsDebugLogPrefix << " config_request"
                    << " result=timeout_timer_start_failed";
                finish_without_showing("config request timeout timer could not start");
                return;
            }

            BOOST_LOG_TRIVIAL(warning)
                << kTlsDebugLogPrefix << " config_request"
                << " transport=winhttp"
                << " certificate_validation="
                << (m_survey_origin.rfind("https://", 0) == 0
                        ? "winhttp_default" : "not_applicable")
                << " origin=" << m_survey_origin
                << " connect_timeout_ms=" << kConfigConnectTimeoutSeconds * 1000
                << " total_timeout_ms=" << kConfigRequestTimeoutSeconds * 1000
                << " response_limit_bytes=" << kConfigResponseLimitBytes;
            m_config_request.Start();
        } catch (const std::exception& exception) {
            BOOST_LOG_TRIVIAL(warning)
                << kTlsDebugLogPrefix << " config_request"
                << " result=start_exception error="
                << sanitized_web_request_error(std::string(exception.what()));
            finish_without_showing("WinHTTP config request could not start");
        }
    }

    void on_config_request_data(wxWebRequestEvent& event)
    {
        if (m_shutting_down || m_startup_window_closed || m_config_response_too_large)
            return;

        const wxWebResponse& response = event.GetResponse();
        if (response.IsOk()) {
            const wxFileOffset content_length = response.GetContentLength();
            if (content_length > static_cast<wxFileOffset>(kConfigResponseLimitBytes)) {
                m_config_response_too_large = true;
                BOOST_LOG_TRIVIAL(warning)
                    << kTlsDebugLogPrefix << " config_request"
                    << " result=response_too_large declared_bytes=" << content_length
                    << " limit_bytes=" << kConfigResponseLimitBytes;
                finish_without_showing("remote config response exceeded size limit");
                return;
            }
        }

        const std::size_t chunk_size = event.GetDataSize();
        if (m_config_response_body.size() > kConfigResponseLimitBytes ||
            chunk_size > kConfigResponseLimitBytes - m_config_response_body.size()) {
            m_config_response_too_large = true;
            BOOST_LOG_TRIVIAL(warning)
                << kTlsDebugLogPrefix << " config_request"
                << " result=response_too_large received_before_bytes="
                << m_config_response_body.size()
                << " chunk_bytes=" << chunk_size
                << " limit_bytes=" << kConfigResponseLimitBytes;
            finish_without_showing("remote config response exceeded size limit");
            return;
        }

        if (chunk_size != 0) {
            m_config_response_body.append(
                static_cast<const char*>(event.GetDataBuffer()), chunk_size);
        }
    }

    void on_config_request_state(wxWebRequestEvent& event)
    {
        const wxWebRequest::State state = event.GetState();
        if (state == wxWebRequest::State_Active || state == wxWebRequest::State_Idle)
            return;

        const wxWebResponse& response = event.GetResponse();
        const unsigned status = response.IsOk() && response.GetStatus() > 0
            ? static_cast<unsigned>(response.GetStatus())
            : 0U;
        const std::size_t received_bytes = m_config_response_body.size();
        std::string body = std::move(m_config_response_body);
        release_config_request();

        if (m_shutting_down || m_startup_window_closed)
            return;

        if (state == wxWebRequest::State_Completed) {
            BOOST_LOG_TRIVIAL(warning)
                << kTlsDebugLogPrefix << " config_request"
                << " result=http_response status=" << status
                << " received_bytes=" << received_bytes
                << " tls_handshake="
                << (m_survey_origin.rfind("https://", 0) == 0 ? "passed" : "not_applicable");
            handle_config_response(std::move(body), status);
            return;
        }

        if (state == wxWebRequest::State_Failed && status >= 400) {
            BOOST_LOG_TRIVIAL(warning)
                << kTlsDebugLogPrefix << " config_request"
                << " result=http_error status=" << status
                << " received_bytes=" << received_bytes
                << " tls_handshake="
                << (m_survey_origin.rfind("https://", 0) == 0 ? "passed" : "not_applicable")
                << " error=" << sanitized_web_request_error(event.GetErrorDescription());
            finish_without_showing("remote config returned an HTTP error");
            return;
        }

        if (state == wxWebRequest::State_Unauthorized) {
            BOOST_LOG_TRIVIAL(warning)
                << kTlsDebugLogPrefix << " config_request"
                << " result=unauthorized status=" << status;
            finish_without_showing("remote config request was unauthorized");
            return;
        }

        if (state == wxWebRequest::State_Cancelled) {
            BOOST_LOG_TRIVIAL(warning)
                << kTlsDebugLogPrefix << " config_request"
                << " result=unexpected_cancel";
            finish_without_showing("remote config request was cancelled");
            return;
        }

        BOOST_LOG_TRIVIAL(warning)
            << kTlsDebugLogPrefix << " config_request"
            << " result=transport_failed status=" << status
            << " error=" << sanitized_web_request_error(event.GetErrorDescription());
        finish_without_showing("remote config request failed");
    }

    void on_config_request_timeout(wxTimerEvent&)
    {
        if (m_shutting_down || m_startup_window_closed || !m_config_request.IsOk() ||
            m_config_request.GetState() != wxWebRequest::State_Active)
            return;
        BOOST_LOG_TRIVIAL(warning)
            << kTlsDebugLogPrefix << " config_request"
            << " result=total_timeout timeout_ms="
            << kConfigRequestTimeoutSeconds * 1000;
        finish_without_showing("remote config request timed out");
    }
#endif

    void release_config_request()
    {
#ifdef _WIN32
        m_config_timeout_timer.Stop();
        m_config_request = wxWebRequest();
        m_config_response_body.clear();
        m_config_response_too_large = false;
#else
        m_config_request.reset();
#endif
    }

    void cancel_config_request(const char* reason)
    {
#ifdef _WIN32
        m_config_timeout_timer.Stop();
        if (m_config_request.IsOk() &&
            m_config_request.GetState() == wxWebRequest::State_Active) {
            BOOST_LOG_TRIVIAL(warning)
                << kTlsDebugLogPrefix << " config_request"
                << " result=cancelled_by_client reason=" << reason;
            m_config_request.Cancel();
        }
        m_config_request = wxWebRequest();
        m_config_response_body.clear();
        m_config_response_too_large = false;
#else
        if (m_config_request != nullptr) {
            m_config_request->cancel();
            m_config_request.reset();
        }
        (void) reason;
#endif
    }

    void initialize_launch_state()
    {
        if (m_launch_state_initialized)
            return;
        m_launch_state_initialized = true;

        const std::time_t now = std::time(nullptr);
        bool dirty = prepare_current_version_state();

        const std::string schema_value = m_app_config.get(m_state_section, "schema_version");
        // An early implementation passed string literals to AppConfig::set().
        // The const-char* values selected its bool overload, producing an
        // impossible all-true initial state.  Repair that state once while
        // preserving the original first-launch timestamp.
        const bool repair_boolean_overload_state = schema_value == "true";
        if (schema_value != "2") {
            m_app_config.set_str(m_state_section, "schema_version", "2");
            dirty = true;
        }

        const std::string first_launch_value =
            m_app_config.get(m_state_section, "first_launch_at_utc");
        const std::string legacy_first_launch_value =
            m_app_config.get(m_state_section, "first_launch_epoch_seconds");
        std::time_t first_launch = parse_config_time(first_launch_value);
        if (first_launch == std::time_t(-1)) {
            first_launch = parse_legacy_epoch_time(legacy_first_launch_value);
            if (first_launch == std::time_t(-1))
                first_launch = now;
            m_app_config.set_str(m_state_section, "first_launch_at_utc",
                                 format_config_time(first_launch));
            dirty = true;
        }
        if (!legacy_first_launch_value.empty()) {
            m_app_config.erase(m_state_section, "first_launch_epoch_seconds");
            dirty = true;
        }

        const std::string stored_count_value =
            m_app_config.get(m_state_section, "successful_print_count");
        const long long stored_count = parse_non_negative_integer(stored_count_value, -1);
        if (repair_boolean_overload_state || stored_count < 0) {
            m_successful_print_count = 0;
            m_app_config.set_str(m_state_section, "successful_print_count", "0");
            dirty = true;
        } else {
            m_successful_print_count = stored_count;
        }

        const std::string consumed_value = m_app_config.get(m_state_section, "consumed");
        m_consumed = repair_boolean_overload_state ? false : parse_bool(consumed_value);
        if (repair_boolean_overload_state || consumed_value.empty()) {
            m_app_config.set(m_state_section, "consumed", false);
            dirty = true;
        }

        const std::string consumed_at_value =
            m_app_config.get(m_state_section, "consumed_at_utc");
        const std::string legacy_consumed_at_value =
            m_app_config.get(m_state_section, "consumed_at_epoch_seconds");
        if (!m_consumed) {
            if (!consumed_at_value.empty())
                m_app_config.erase(m_state_section, "consumed_at_utc");
            dirty = dirty || !consumed_at_value.empty();
        } else if (parse_config_time(consumed_at_value) == std::time_t(-1)) {
            const std::time_t legacy_consumed_at = parse_legacy_epoch_time(legacy_consumed_at_value);
            if (legacy_consumed_at != std::time_t(-1))
                m_app_config.set_str(m_state_section, "consumed_at_utc",
                                     format_config_time(legacy_consumed_at));
            else if (!consumed_at_value.empty())
                m_app_config.erase(m_state_section, "consumed_at_utc");
            dirty = dirty || legacy_consumed_at != std::time_t(-1) || !consumed_at_value.empty();
        }
        if (!legacy_consumed_at_value.empty()) {
            m_app_config.erase(m_state_section, "consumed_at_epoch_seconds");
            dirty = true;
        }

        // This immutable snapshot is the key to "qualifies this session, shows
        // next launch": record_successful_print never recalculates it.
        m_eligible_this_launch = !m_consumed &&
            m_successful_print_count >= kRequiredSuccessfulPrints &&
            now >= first_launch && now - first_launch > kRequiredUsageSeconds;

#ifdef SATISFACTION_SURVEY_LOCAL_DEBUG
        const long long usage_seconds = static_cast<long long>(std::difftime(now, first_launch));
        BOOST_LOG_TRIVIAL(warning)
            << "[SAT_SURVEY_DEBUG][SatisfactionSurvey] eligibility"
            << " version=" << m_full_version
            << " channel=" << m_channel
            << " consumed=" << (m_consumed ? "true" : "false")
            << " print_count=" << m_successful_print_count
            << " first_launch_at_utc=" << format_config_time(first_launch)
            << " usage_seconds=" << usage_seconds
            << " required_usage_seconds=" << kRequiredUsageSeconds
            << " eligible=" << (m_eligible_this_launch ? "true" : "false");
#endif

        if (dirty)
            save_config("initial survey state");
    }

    bool prepare_current_version_state()
    {
        bool dirty = false;
        const std::string legacy_section =
            m_app_config.get("app", kLegacySurveyStateSectionIndexKey);
        if (!legacy_section.empty()) {
            if (legacy_section.compare(0, sizeof(kLegacySurveyStateSectionPrefix) - 1,
                                       kLegacySurveyStateSectionPrefix) == 0 &&
                m_app_config.has_section(legacy_section)) {
                m_app_config.clear_section(legacy_section);
            }
            m_app_config.erase("app", kLegacySurveyStateSectionIndexKey);
            dirty = true;
        }

        if (!m_app_config.has_section(m_state_section)) {
#ifdef SATISFACTION_SURVEY_LOCAL_DEBUG
            BOOST_LOG_TRIVIAL(warning)
                << "[SAT_SURVEY_DEBUG][SatisfactionSurvey] version_state action=initialize"
                << " current_version=" << m_full_version
                << " current_channel=" << m_channel;
#endif
            m_app_config.set_str(m_state_section, "app_version", m_full_version);
            m_app_config.set_str(m_state_section, "channel", m_channel);
            return true;
        }

        const std::string stored_version =
            m_app_config.get(m_state_section, "app_version");
        const std::string stored_channel =
            m_app_config.get(m_state_section, "channel");
        const bool same_version =
            stored_version == m_full_version && stored_channel == m_channel;
        if (same_version) {
#ifdef SATISFACTION_SURVEY_LOCAL_DEBUG
            BOOST_LOG_TRIVIAL(warning)
                << "[SAT_SURVEY_DEBUG][SatisfactionSurvey] version_state action=keep"
                << " current_version=" << m_full_version
                << " current_channel=" << m_channel;
#endif
            return dirty;
        }

#ifdef SATISFACTION_SURVEY_LOCAL_DEBUG
        BOOST_LOG_TRIVIAL(warning)
            << "[SAT_SURVEY_DEBUG][SatisfactionSurvey] version_state action=reset"
            << " stored_version=" << stored_version
            << " stored_channel=" << stored_channel
            << " current_version=" << m_full_version
            << " current_channel=" << m_channel;
#endif
        m_app_config.clear_section(m_state_section);
        m_app_config.set_str(m_state_section, "app_version", m_full_version);
        m_app_config.set_str(m_state_section, "channel", m_channel);
        BOOST_LOG_TRIVIAL(info)
            << "[SatisfactionSurvey] reset state after version change";
        return true;
    }

    void debug_print_decision(SatisfactionSurveyPrintSource source, const char* decision) const
    {
        BOOST_LOG_TRIVIAL(warning)
            << "[SAT_SURVEY_DEBUG][SatisfactionSurvey] print_event"
            << " source=" << print_source_name(source)
            << " decision=" << decision
            << " current_count=" << m_successful_print_count;
    }

    void debug_startup_gate(const char* reason)
    {
#ifdef SATISFACTION_SURVEY_LOCAL_DEBUG
        if (m_last_debug_startup_gate == reason)
            return;
        m_last_debug_startup_gate = reason;
        BOOST_LOG_TRIVIAL(warning)
            << "[SAT_SURVEY_DEBUG][SatisfactionSurvey] startup_gate=" << reason;
#else
        (void) reason;
#endif
    }

    void handle_config_response(std::string body, unsigned status)
    {
        release_config_request();
        if (m_startup_window_closed || m_shutting_down)
            return;
#ifdef SATISFACTION_SURVEY_LOCAL_DEBUG
        BOOST_LOG_TRIVIAL(warning)
            << "[SAT_SURVEY_DEBUG][SatisfactionSurvey] remote_config status=" << status;
#endif
        if (status < 200 || status >= 300) {
            BOOST_LOG_TRIVIAL(warning)
                << kTlsDebugLogPrefix << " config_validation"
                << " result=non_success_status status=" << status;
            finish_without_showing("remote config returned a non-success status");
            return;
        }

        try {
            const nlohmann::json config = nlohmann::json::parse(body);
            if (!config.is_object() || config.value("schema_version", 0) != 1 ||
                config.value("survey_key", std::string()) != "software_satisfaction" ||
                config.value("target_version", std::string()) != m_full_version ||
                config.value("target_channel", std::string()) != m_channel ||
                !config.contains("enabled") || !config["enabled"].is_boolean()) {
                BOOST_LOG_TRIVIAL(warning)
                    << kTlsDebugLogPrefix << " config_validation"
                    << " result=contract_mismatch";
                finish_without_showing("remote config contract mismatch");
                return;
            }

            if (!config["enabled"].get<bool>()) {
                m_remote_state = RemoteState::Disabled;
                BOOST_LOG_TRIVIAL(warning)
                    << kTlsDebugLogPrefix << " config_validation"
                    << " result=survey_disabled";
                finish_without_showing("survey disabled for this version");
                return;
            }

            const std::string page_url = config.value("page_url", std::string());
            if (!is_satisfaction_survey_url_from_origin(page_url, m_survey_origin)) {
                BOOST_LOG_TRIVIAL(warning)
                    << kTlsDebugLogPrefix << " config_validation"
                    << " result=untrusted_page_url";
                finish_without_showing("remote config supplied an untrusted page URL");
                return;
            }

            const std::string locale = m_app_config.get("language").empty()
                ? std::string("en_GB")
                : m_app_config.get("language");
            std::ostringstream page_query;
            page_query << "product=creality-print"
                        << "&app_version=" << Http::url_encode(m_full_version)
                        << "&channel=" << Http::url_encode(m_channel)
                        << "&platform=" << survey_platform_name()
                        << "&locale=" << Http::url_encode(locale)
                        << "&region=" << Http::url_encode(m_app_config.get("region"))
                        << "&theme=" << (wxGetApp().dark_mode() ? "dark" : "light");

            m_page_url = append_query(page_url, page_query.str());
            m_remote_state = RemoteState::Enabled;
            BOOST_LOG_TRIVIAL(warning)
                << kTlsDebugLogPrefix << " config_validation"
                << " result=accepted enabled=true trusted_page=true";
#ifdef SATISFACTION_SURVEY_LOCAL_DEBUG
            BOOST_LOG_TRIVIAL(warning)
                << "[SAT_SURVEY_DEBUG][SatisfactionSurvey] remote_config enabled=true trusted_page=true";
#endif
            ensure_dialog_loading();
        } catch (const std::exception& exception) {
            BOOST_LOG_TRIVIAL(warning)
                << kTlsDebugLogPrefix << " config_validation"
                << " result=invalid_json";
            BOOST_LOG_TRIVIAL(warning) << "[SatisfactionSurvey] invalid remote config: "
                                       << exception.what();
            finish_without_showing("remote config JSON was invalid");
        }
    }

    void ensure_dialog_loading()
    {
        wxWindow* parent = m_parent.get();
        if (m_remote_state != RemoteState::Enabled || !m_post_init_finished ||
            m_startup_window_closed || parent == nullptr || m_dialog != nullptr)
            return;

        std::weak_ptr<int> lifetime = m_async_lifetime;
        try {
#ifdef SATISFACTION_SURVEY_LOCAL_DEBUG
            BOOST_LOG_TRIVIAL(warning)
                << "[SAT_SURVEY_DEBUG][SatisfactionSurvey] webview loading_started";
#endif
            m_dialog = new SatisfactionSurveyDialog(
                parent,
                wxString::FromUTF8(m_page_url.c_str()),
                m_survey_origin,
                [this, lifetime]() {
                    if (!lifetime.expired()) {
                        m_page_ready = true;
                        m_quiet_since = {};
#ifdef SATISFACTION_SURVEY_LOCAL_DEBUG
                        BOOST_LOG_TRIVIAL(warning)
                            << "[SAT_SURVEY_DEBUG][SatisfactionSurvey] webview page_ready=true";
#endif
                    }
                },
                [this, lifetime]() {
                    if (!lifetime.expired())
                        finish_without_showing("survey page failed before display");
                });
            if (m_dialog->load_failed()) {
                finish_without_showing("survey WebView is unavailable");
                return;
            }
            m_dialog->start_loading();
        } catch (const std::exception& exception) {
            BOOST_LOG_TRIVIAL(warning) << "[SatisfactionSurvey] survey WebView could not be created: "
                                       << exception.what();
            finish_without_showing("survey WebView could not be created");
        }
    }

    void on_timer(wxTimerEvent&)
    {
        if (m_startup_window_closed || m_shutting_down) {
            m_timer.Stop();
            return;
        }

        const auto now = std::chrono::steady_clock::now();
        if (now >= m_deadline) {
            finish_without_showing("startup display window expired");
            return;
        }

        ensure_dialog_loading();
        if (!m_startup_tail_released) {
            debug_startup_gate("waiting_startup_tail");
            return;
        }
        if (m_remote_state != RemoteState::Enabled) {
            debug_startup_gate("waiting_remote_config");
            return;
        }
        if (!m_page_ready) {
            debug_startup_gate("waiting_page_ready");
            return;
        }
        if (m_dialog == nullptr) {
            debug_startup_gate("waiting_dialog");
            return;
        }

        if (m_user_activity_during_startup) {
            finish_without_showing("print activity started during the startup window");
            return;
        }

        Plater* plater = wxGetApp().plater();
        if (plater != nullptr && plater->is_background_process_slicing()) {
            finish_without_showing("slicing started during the startup window");
            return;
        }

        wxWindow* parent = m_parent.get();
        auto* top_level = dynamic_cast<wxTopLevelWindow*>(parent);
        if (top_level == nullptr || parent->IsBeingDeleted()) {
            finish_without_showing("main window is unavailable");
            return;
        }

        const bool another_modal = has_other_modal_dialog(m_dialog.get());
        if (!another_modal && now - m_post_init_time >= std::chrono::milliseconds(kInactiveGraceMilliseconds) &&
            (!top_level->IsShownOnScreen() || top_level->IsIconized() || !top_level->IsActive())) {
            finish_without_showing("main window is inactive or minimized");
            return;
        }

        if (!can_show_now()) {
            debug_startup_gate(another_modal ? "waiting_other_modal" : "waiting_native_ui");
            m_quiet_since = {};
            return;
        }

        if (m_quiet_since.time_since_epoch().count() == 0) {
            debug_startup_gate("quiet_window_started");
            m_quiet_since = now;
            return;
        }
        if (now - m_quiet_since < std::chrono::milliseconds(kQuietWindowMilliseconds))
            return;

        debug_startup_gate("ready_to_show");
        show_survey();
    }

    bool can_show_now() const
    {
        return !m_shutting_down && !WebView::IsShuttingDown() &&
               wxGetApp().can_show_satisfaction_survey();
    }

    void show_survey()
    {
        if (m_dialog == nullptr || !m_dialog->page_ready() || m_startup_window_closed)
            return;

        const std::string shown_context =
            survey_shown_context(m_app_config, m_full_version, m_channel)
                .dump(-1, ' ', true, nlohmann::json::error_handler_t::replace);
        if (!m_dialog->set_shown_context(shown_context)) {
            finish_without_showing("survey display context was unavailable");
            return;
        }

        const std::string old_consumed = m_app_config.get(m_state_section, "consumed");
        const std::string old_consumed_at =
            m_app_config.get(m_state_section, "consumed_at_utc");
        m_app_config.set(m_state_section, "consumed", true);
        m_app_config.set_str(m_state_section, "consumed_at_utc",
                             format_config_time(std::time(nullptr)));
        try {
            m_app_config.save();
        } catch (const std::exception& exception) {
            m_app_config.set(m_state_section, "consumed",
                             old_consumed.empty() ? std::string("false") : old_consumed);
            if (old_consumed_at.empty())
                m_app_config.erase(m_state_section, "consumed_at_utc");
            else
                m_app_config.set_str(m_state_section, "consumed_at_utc", old_consumed_at);
            BOOST_LOG_TRIVIAL(error) << "[SatisfactionSurvey] refusing to display because consumed state could not be saved: "
                                     << exception.what();
            finish_without_showing("consumed state persistence failed");
            return;
        }

        m_consumed = true;
        m_startup_window_closed = true;
        m_timer.Stop();
        cancel_config_request("survey_showing");

        SatisfactionSurveyDialog* dialog = m_dialog;
        m_dialog = nullptr;
        m_active_dialog = dialog;
        dialog->CentreOnParent();
        BOOST_LOG_TRIVIAL(info) << "[SatisfactionSurvey] displaying survey for version "
                                << m_full_version << " (" << m_channel << ')';
#ifdef SATISFACTION_SURVEY_LOCAL_DEBUG
        const wxRect dialog_rect = dialog->GetScreenRect();
        const wxRect parent_rect = dialog->GetParent() != nullptr
            ? dialog->GetParent()->GetScreenRect()
            : wxRect();
        auto* parent_top_level = dynamic_cast<wxTopLevelWindow*>(dialog->GetParent());
        BOOST_LOG_TRIVIAL(warning)
            << "[SAT_SURVEY_DEBUG][SatisfactionSurvey] dialog show_enter"
            << " dialog_rect=" << dialog_rect.GetX() << ',' << dialog_rect.GetY()
            << ',' << dialog_rect.GetWidth() << ',' << dialog_rect.GetHeight()
            << " parent_rect=" << parent_rect.GetX() << ',' << parent_rect.GetY()
            << ',' << parent_rect.GetWidth() << ',' << parent_rect.GetHeight()
            << " parent_shown="
            << (dialog->GetParent() != nullptr && dialog->GetParent()->IsShownOnScreen()
                    ? "true" : "false")
            << " parent_active="
            << (parent_top_level != nullptr && parent_top_level->IsActive() ? "true" : "false")
            << " parent_iconized="
            << (parent_top_level != nullptr && parent_top_level->IsIconized() ? "true" : "false");
#endif
        wxWeakRef<SatisfactionSurveyDialog> shown_dialog(dialog);
        const int modal_result = dialog->ShowModal();
#ifdef SATISFACTION_SURVEY_LOCAL_DEBUG
        BOOST_LOG_TRIVIAL(warning)
            << "[SAT_SURVEY_DEBUG][SatisfactionSurvey] dialog closed modal_result="
            << modal_result;
#else
        (void) modal_result;
#endif
        m_active_dialog = nullptr;
        if (SatisfactionSurveyDialog* finished_dialog = shown_dialog.get();
            finished_dialog != nullptr && !finished_dialog->IsBeingDeleted()) {
            finished_dialog->Destroy();
        }
    }

    void finish_without_showing(const char* reason)
    {
        if (m_startup_window_closed || m_shutting_down)
            return;
#ifdef SATISFACTION_SURVEY_LOCAL_DEBUG
        BOOST_LOG_TRIVIAL(warning)
            << "[SAT_SURVEY_DEBUG][SatisfactionSurvey] launch_finished shown=false reason="
            << reason;
#endif
        BOOST_LOG_TRIVIAL(info) << "[SatisfactionSurvey] no survey this launch: " << reason;
        m_startup_window_closed = true;
        m_timer.Stop();
        cancel_config_request("launch_finished");
        destroy_dialog();
    }

    void destroy_dialog()
    {
        if (m_dialog == nullptr)
            return;
        SatisfactionSurveyDialog* dialog = m_dialog;
        m_dialog = nullptr;
        if (!dialog->IsBeingDeleted())
            dialog->Destroy();
    }

    void save_config(const char* context)
    {
        try {
            m_app_config.save();
        } catch (const std::exception& exception) {
            BOOST_LOG_TRIVIAL(error) << "[SatisfactionSurvey] failed to save " << context
                                     << ": " << exception.what();
        }
    }

    AppConfig& m_app_config;
    std::string m_full_version;
    std::string m_channel;
    std::string m_state_section;
    CxAgentEndpointSelection m_endpoint_selection;
    std::string m_survey_origin;

    long long m_successful_print_count {0};
    bool m_consumed {false};
    bool m_launch_state_initialized {false};
    bool m_eligible_this_launch {false};
    bool m_check_started {false};
    bool m_post_init_finished {false};
    bool m_waiting_for_startup_restore {false};
    bool m_startup_tail_released {false};
    bool m_startup_window_closed {false};
    bool m_user_activity_during_startup {false};
    bool m_page_ready {false};
    bool m_shutting_down {false};

    RemoteState m_remote_state {RemoteState::Unknown};
#ifdef SATISFACTION_SURVEY_LOCAL_DEBUG
    std::string m_last_debug_startup_gate;
#endif
    std::string m_page_url;
#ifdef _WIN32
    wxWebSession m_config_session;
    std::string m_config_response_body;
    bool m_config_response_too_large {false};
    wxWebRequest m_config_request;
#else
    Http::Ptr m_config_request;
#endif
    std::shared_ptr<int> m_async_lifetime {std::make_shared<int>(0)};

    wxTimer m_timer;
#ifdef _WIN32
    wxTimer m_config_timeout_timer;
#endif
    wxWeakRef<wxWindow> m_parent;
    wxWeakRef<SatisfactionSurveyDialog> m_dialog;
    wxWeakRef<SatisfactionSurveyDialog> m_active_dialog;
    std::chrono::steady_clock::time_point m_post_init_time;
    std::chrono::steady_clock::time_point m_deadline;
    std::chrono::steady_clock::time_point m_quiet_since;
    std::map<std::string, std::chrono::steady_clock::time_point> m_recent_prints;
};

SatisfactionSurveyManager::SatisfactionSurveyManager(AppConfig& app_config,
                                                     std::string full_version,
                                                     std::string channel)
    : m_impl(std::make_unique<Impl>(app_config, std::move(full_version), std::move(channel)))
{
}

SatisfactionSurveyManager::~SatisfactionSurveyManager() = default;

void SatisfactionSurveyManager::begin_startup_check()
{
    m_impl->begin_startup_check();
}

void SatisfactionSurveyManager::expect_startup_restore()
{
    m_impl->expect_startup_restore();
}

void SatisfactionSurveyManager::notify_startup_restore_finished()
{
    m_impl->notify_startup_restore_finished();
}

void SatisfactionSurveyManager::notify_post_init_finished(wxWindow* parent)
{
    m_impl->notify_post_init_finished(parent);
}

void SatisfactionSurveyManager::record_successful_print(const SatisfactionSurveyPrintEvent& event)
{
    m_impl->record_successful_print(event);
}

void SatisfactionSurveyManager::shutdown()
{
    m_impl->shutdown();
}

} // namespace GUI
} // namespace Slic3r
