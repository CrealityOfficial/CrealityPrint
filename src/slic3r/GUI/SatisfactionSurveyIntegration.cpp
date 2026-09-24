#include "SatisfactionSurveyIntegration.hpp"

#include "GUI_App.hpp"
#include "SatisfactionSurveyManager.hpp"
#include "print_manage/App/PrinterMgrView.hpp"
#include "print_manage/App/SendToPrinter.hpp"
#include "print_manage/data/DataCenter.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cstdint>
#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

#include <wx/eventfilter.h>
#include <wx/webview.h>

namespace Slic3r {
namespace GUI {

namespace {

using json = nlohmann::json;

const json* json_object(const json& value)
{
    if (value.is_object())
        return &value;
    return nullptr;
}

const json* child_object(const json& parent, const char* key, json& parsed_string)
{
    if (!parent.is_object())
        return nullptr;

    const auto it = parent.find(key);
    if (it == parent.end() || it->is_null())
        return nullptr;
    if (it->is_object())
        return &*it;
    if (!it->is_string())
        return nullptr;

    try {
        parsed_string = json::parse(it->get<std::string>());
        return json_object(parsed_string);
    } catch (...) {
        return nullptr;
    }
}

std::string first_string(const json& object, std::initializer_list<const char*> keys)
{
    if (!object.is_object())
        return {};

    for (const char* key : keys) {
        const auto it = object.find(key);
        if (it != object.end() && it->is_string() && !it->get_ref<const std::string&>().empty())
            return it->get<std::string>();
    }
    return {};
}

void append_addresses(const json& object, const char* key, std::vector<std::string>& addresses)
{
    if (!object.is_object())
        return;

    const auto it = object.find(key);
    if (it == object.end() || !it->is_array())
        return;

    for (const json& item : *it) {
        if (!item.is_string())
            continue;
        const std::string& address = item.get_ref<const std::string&>();
        if (!address.empty() && std::find(addresses.begin(), addresses.end(), address) == addresses.end())
            addresses.emplace_back(address);
    }
}

std::string join_fingerprint(const std::vector<std::string>& values)
{
    std::string result;
    for (const std::string& value : values) {
        if (value.empty())
            continue;
        if (!result.empty())
            result += '|';
        result += value;
    }
    return result;
}

SatisfactionSurveyManager* survey_manager()
{
    return wxTheApp == nullptr ? nullptr : wxGetApp().satisfaction_survey_manager();
}

bool is_print_workflow_webview(const wxWebViewEvent& event)
{
    auto* window = dynamic_cast<wxWindow*>(event.GetEventObject());
    while (window != nullptr) {
        if (dynamic_cast<PrinterMgrView*>(window) != nullptr ||
            dynamic_cast<CxSentToPrinterDialog*>(window) != nullptr)
            return true;
        window = window->GetParent();
    }
    return false;
}

} // namespace

// This filter observes the existing, already-successful print navigation
// messages.  It intentionally does not change any print workflow itself.
class SatisfactionSurveyEventFilter final : public wxEventFilter
{
public:
    int FilterEvent(wxEvent& event) override
    {
        if (event.GetEventType() != wxEVT_WEBVIEW_SCRIPT_MESSAGE_RECEIVED)
            return wxEventFilter::Event_Skip;

        auto* web_event = dynamic_cast<wxWebViewEvent*>(&event);
        if (web_event == nullptr || !is_print_workflow_webview(*web_event))
            return wxEventFilter::Event_Skip;

        try {
            const json message = json::parse(web_event->GetString().ToUTF8().data());
            const std::string command = message.value("command", std::string());

            if (command == "set_device_detail_state")
                observe_device_detail_state(message, web_event->GetEventObject());
            else if (command == "device_detail_print")
                observe_device_detail_print(message, web_event->GetEventObject());
            else if (command == "forward_device_detail")
                observe_forward_device_detail(message);
        } catch (...) {
            // This is a passive observer.  Malformed or unrelated page messages
            // must keep their existing behavior and must never affect printing.
        }

        return wxEventFilter::Event_Skip;
    }

    void notify_multi_device_upload_result(const std::string& device_address, bool succeeded)
    {
        if (device_address.empty())
            return;

        const auto operation = std::find_if(
            m_pending_multi_device_operations.begin(),
            m_pending_multi_device_operations.end(),
            [&device_address](const PendingMultiDeviceOperation& candidate) {
                return std::find(candidate.remaining_addresses.begin(),
                                 candidate.remaining_addresses.end(),
                                 device_address) != candidate.remaining_addresses.end();
            });
        if (operation == m_pending_multi_device_operations.end())
            return;

        const auto address = std::find(operation->remaining_addresses.begin(),
                                       operation->remaining_addresses.end(),
                                       device_address);
        if (!succeeded) {
            operation->remaining_addresses.erase(address);
            if (operation->remaining_addresses.empty())
                m_pending_multi_device_operations.erase(operation);
            return;
        }

        SatisfactionSurveyManager* manager = survey_manager();
        if (manager == nullptr)
            return;

        const DM::Device device = DM::DataCenter::Ins().get_printer_data(device_address);
        if (!device.valid || (device.deviceType != 0 && device.deviceType != 1)) {
            operation->remaining_addresses.erase(address);
            if (operation->remaining_addresses.empty())
                m_pending_multi_device_operations.erase(operation);
            return;
        }

        SatisfactionSurveyPrintEvent print_event;
        print_event.source = SatisfactionSurveyPrintSource::MultiDevice;
        print_event.device_addresses.emplace_back(device_address);
        print_event.operation_fingerprint = operation->fingerprint;
        manager->record_successful_print(print_event);

        // One multi-device click is one print operation. Once any eligible
        // device reaches upload completion, later devices and delayed batches
        // from the same click must not add another count.
        m_pending_multi_device_operations.erase(operation);
    }

private:
    struct PendingMultiDeviceOperation
    {
        std::vector<std::string> remaining_addresses;
        std::string              fingerprint;
    };

    void observe_device_detail_state(const json& message, const wxObject* event_source)
    {
        m_detail_visible = message.value("visible", false);
        m_detail_address = m_detail_visible ? first_string(message, {"address", "ip"}) : std::string();
        m_detail_event_source = m_detail_visible ? event_source : nullptr;
    }

    void observe_device_detail_print(const json& message, const wxObject* event_source) const
    {
        SatisfactionSurveyManager* manager = survey_manager();
        if (manager == nullptr || !m_detail_visible || m_detail_address.empty() ||
            event_source != m_detail_event_source)
            return;

        SatisfactionSurveyPrintEvent print_event;
        print_event.source = SatisfactionSurveyPrintSource::DeviceDetail;
        print_event.device_addresses.emplace_back(m_detail_address);
        // A calibration print emits this lifecycle message twice with different
        // metadata for one click.  Address-only debounce keeps it to one count.
        print_event.operation_fingerprint = m_detail_address;
        manager->record_successful_print(print_event);
    }

    void observe_forward_device_detail(const json& message)
    {
        SatisfactionSurveyManager* manager = survey_manager();
        if (manager == nullptr)
            return;

        // Ordinary single-printer flow: successful print followed by opening
        // the target device details.  Third-party devices are filtered again by
        // SatisfactionSurveyManager through DataCenter/deviceType.
        const std::string direct_address = first_string(message, {"ip", "address"});
        if (!direct_address.empty()) {
            const auto force_refresh = message.find("forceRefresh");
            if (force_refresh == message.end() || !force_refresh->is_boolean() ||
                !force_refresh->get<bool>())
                return;

            SatisfactionSurveyPrintEvent print_event;
            print_event.source = SatisfactionSurveyPrintSource::SendToPrinter;
            print_event.device_addresses.emplace_back(direct_address);
            print_event.operation_fingerprint = join_fingerprint({
                direct_address, first_string(message, {"fileName", "filename", "name"})
            });
            manager->record_successful_print(print_event);
            return;
        }

        // Multi-device flow wraps its payload twice. action=print only records
        // a pending operation here: the device page opens before transmission,
        // so counting this navigation would also count cancelled uploads.
        json parsed_level_one;
        const json* level_one = child_object(message, "data", parsed_level_one);
        if (level_one == nullptr)
            return;

        json parsed_level_two;
        const json* payload = child_object(*level_one, "data", parsed_level_two);
        if (payload == nullptr)
            payload = level_one;
        if (payload->value("action", std::string()) != "print")
            return;

        std::vector<std::string> addresses;
        append_addresses(*payload, "deviceIps", addresses);
        append_addresses(*payload, "device_ips", addresses);
        const std::string single_address = first_string(*payload, {"ip", "address"});
        if (!single_address.empty() &&
            std::find(addresses.begin(), addresses.end(), single_address) == addresses.end())
            addresses.emplace_back(single_address);
        if (addresses.empty())
            return;

        std::vector<std::string> fingerprint_addresses = addresses;
        std::sort(fingerprint_addresses.begin(), fingerprint_addresses.end());

        fingerprint_addresses.emplace_back(first_string(*payload, {"fileName", "filename", "name"}));
        fingerprint_addresses.emplace_back(std::to_string(++m_next_multi_device_operation_id));

        PendingMultiDeviceOperation operation;
        operation.remaining_addresses = std::move(addresses);
        operation.fingerprint = join_fingerprint(fingerprint_addresses);
        m_pending_multi_device_operations.emplace_back(std::move(operation));
    }

    bool        m_detail_visible {false};
    std::string m_detail_address;
    const wxObject* m_detail_event_source {nullptr};
    std::vector<PendingMultiDeviceOperation> m_pending_multi_device_operations;
    std::uint64_t m_next_multi_device_operation_id {0};
};

namespace {

SatisfactionSurveyEventFilter* g_satisfaction_survey_event_filter {nullptr};

} // namespace

void install_satisfaction_survey_event_filter()
{
    if (g_satisfaction_survey_event_filter == nullptr) {
        g_satisfaction_survey_event_filter = new SatisfactionSurveyEventFilter();
        wxEvtHandler::AddFilter(g_satisfaction_survey_event_filter);
    }
}

void uninstall_satisfaction_survey_event_filter()
{
    if (g_satisfaction_survey_event_filter != nullptr) {
        wxEvtHandler::RemoveFilter(g_satisfaction_survey_event_filter);
        delete g_satisfaction_survey_event_filter;
        g_satisfaction_survey_event_filter = nullptr;
    }
}

void notify_satisfaction_survey_multi_device_upload_result(const std::string& device_address,
                                                           bool               succeeded)
{
    if (g_satisfaction_survey_event_filter == nullptr)
        return;

    if (!wxIsMainThread()) {
        if (wxTheApp != nullptr) {
            wxTheApp->CallAfter([device_address, succeeded]() {
                notify_satisfaction_survey_multi_device_upload_result(device_address, succeeded);
            });
        }
        return;
    }

    g_satisfaction_survey_event_filter->notify_multi_device_upload_result(device_address, succeeded);
}

void record_ai_satisfaction_survey_print(const std::string& device_address,
                                         const std::string& operation_id)
{
    SatisfactionSurveyManager* manager = survey_manager();
    if (manager == nullptr || device_address.empty())
        return;

    SatisfactionSurveyPrintEvent print_event;
    print_event.source = SatisfactionSurveyPrintSource::AiAssistant;
    print_event.device_addresses.emplace_back(device_address);
    print_event.operation_fingerprint = join_fingerprint({device_address, operation_id});
    manager->record_successful_print(print_event);
}

} // namespace GUI
} // namespace Slic3r
