#include "SatisfactionSurveyDialog.hpp"

#include "GUI_App.hpp"
#include "Widgets/WebView.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <utility>

#include <boost/log/trivial.hpp>

#include <wx/display.h>
#include <wx/event.h>
#include <wx/sizer.h>
#include <wx/webview.h>

namespace Slic3r {
namespace GUI {

namespace {

constexpr int kDialogWidthDip = 640;
// Keep the legacy height only as a hidden-page compatibility fallback.  The
// current survey reports its measured height before survey_ready, so users see
// the compact content-sized window instead of this fallback.
constexpr int kDefaultDialogHeightDip = 680;
constexpr int kMinimumDialogWidthDip = 620;
constexpr int kMinimumDialogHeightDip = 240;
constexpr int kWorkAreaMarginDip = 64;
constexpr int kMaximumRequestedHeightDip = 2000;

struct SurveyMessage
{
    std::string command;
    int requested_height_dip {0};
};

SurveyMessage parse_survey_message(const wxString& raw_message)
{
    const std::string message = raw_message.ToUTF8().data();
    if (message == "survey_ready" || message == "survey_closed" ||
        message == "survey_submitted" || message == "survey_error")
        return {message, 0};

    try {
        const nlohmann::json payload = nlohmann::json::parse(message);
        if (!payload.is_object())
            return {};

        const auto command_it = payload.find("command");
        if (command_it == payload.end() || !command_it->is_string())
            return {};

        SurveyMessage parsed {command_it->get<std::string>(), 0};
        if (parsed.command != "survey_resize")
            return parsed;

        const auto height_it = payload.find("height");
        if (height_it == payload.end())
            return {};

        std::uint64_t requested_height = 0;
        if (height_it->is_number_unsigned()) {
            requested_height = height_it->get<std::uint64_t>();
        } else if (height_it->is_number_integer()) {
            const std::int64_t signed_height = height_it->get<std::int64_t>();
            if (signed_height <= 0)
                return {};
            requested_height = static_cast<std::uint64_t>(signed_height);
        } else {
            return {};
        }

        if (requested_height == 0 || requested_height > kMaximumRequestedHeightDip)
            return {};
        parsed.requested_height_dip = static_cast<int>(requested_height);
        return parsed;
    } catch (const std::exception&) {
        // Lifecycle messages are intentionally small.  Invalid/unrelated
        // messages are ignored without logging their body, which may contain
        // user-entered survey data in a future page implementation.
    }
    return {};
}

wxRect display_work_area(wxWindow* window)
{
    int display_index = wxDisplay::GetFromWindow(window);
    if (display_index == wxNOT_FOUND && window->GetParent() != nullptr)
        display_index = wxDisplay::GetFromWindow(window->GetParent());
    return wxDisplay(display_index == wxNOT_FOUND ? 0u :
                     static_cast<unsigned int>(display_index)).GetClientArea();
}

} // namespace

bool is_satisfaction_survey_url_from_origin(const std::string& url,
                                            const std::string& trusted_origin)
{
    if (trusted_origin.empty())
        return false;

    std::string normalized_url = url;
    std::string normalized_origin = trusted_origin;
    const auto to_lower = [](unsigned char c) { return static_cast<char>(std::tolower(c)); };
    std::transform(normalized_url.begin(), normalized_url.end(), normalized_url.begin(), to_lower);
    std::transform(normalized_origin.begin(), normalized_origin.end(), normalized_origin.begin(), to_lower);

    while (!normalized_origin.empty() && normalized_origin.back() == '/')
        normalized_origin.pop_back();
    if (normalized_origin.empty() ||
        normalized_url.compare(0, normalized_origin.size(), normalized_origin) != 0)
        return false;
    if (normalized_url.size() == normalized_origin.size())
        return true;

    const char boundary = normalized_url[normalized_origin.size()];
    return boundary == '/' || boundary == '?' || boundary == '#';
}

SatisfactionSurveyDialog::SatisfactionSurveyDialog(wxWindow* parent,
                                                   const wxString& page_url,
                                                   std::string trusted_origin,
                                                   Callback on_ready,
                                                   Callback on_failed)
    : DPIDialog(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE)
    , m_page_url(page_url)
    , m_trusted_origin(std::move(trusted_origin))
    , m_on_ready(std::move(on_ready))
    , m_on_failed(std::move(on_failed))
{
    SetBackgroundColour(wxGetApp().dark_mode() ? wxColour("#4B4B4D") : *wxWHITE);
    m_requested_height_dip = kDefaultDialogHeightDip;
    apply_dialog_size();

    if (!is_satisfaction_survey_url_from_origin(m_page_url.ToUTF8().data(), m_trusted_origin)) {
        m_failed = true;
        return;
    }

    m_browser = WebView::CreateWebView(this, wxEmptyString);
    if (m_browser == nullptr) {
        m_failed = true;
        return;
    }

    auto* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(m_browser, 1, wxEXPAND);
    SetSizer(sizer);

    m_browser->Bind(wxEVT_WEBVIEW_ERROR, &SatisfactionSurveyDialog::on_error, this);
    m_browser->Bind(wxEVT_WEBVIEW_NAVIGATING, &SatisfactionSurveyDialog::on_navigating, this);
    m_browser->Bind(wxEVT_WEBVIEW_NEWWINDOW, &SatisfactionSurveyDialog::on_new_window, this);
    m_browser->Bind(wxEVT_WEBVIEW_SCRIPT_MESSAGE_RECEIVED,
                    &SatisfactionSurveyDialog::on_script_message,
                    this);
    Bind(wxEVT_SHOW, &SatisfactionSurveyDialog::on_show, this);

    Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent& event) {
        if (IsModal()) {
            EndModal(wxID_CANCEL);
            return;
        }
        signal_failed();
        event.Skip();
    });

    CentreOnParent();
}

SatisfactionSurveyDialog::~SatisfactionSurveyDialog()
{
    if (m_browser != nullptr) {
        m_browser->SetEvtHandlerEnabled(false);
        m_browser->Unbind(wxEVT_WEBVIEW_ERROR, &SatisfactionSurveyDialog::on_error, this);
        m_browser->Unbind(wxEVT_WEBVIEW_NAVIGATING, &SatisfactionSurveyDialog::on_navigating, this);
        m_browser->Unbind(wxEVT_WEBVIEW_NEWWINDOW, &SatisfactionSurveyDialog::on_new_window, this);
        m_browser->Unbind(wxEVT_WEBVIEW_SCRIPT_MESSAGE_RECEIVED,
                          &SatisfactionSurveyDialog::on_script_message,
                          this);
    }
    Unbind(wxEVT_SHOW, &SatisfactionSurveyDialog::on_show, this);
}

void SatisfactionSurveyDialog::start_loading()
{
    if (m_loading_started || m_failed)
        return;

    m_loading_started = true;
    // On WebKit the default "wx" message handler is registered asynchronously.
    // Queue navigation behind that registration to avoid losing survey_ready.
    CallAfter([this]() {
        if (m_browser == nullptr || m_failed || m_browser->IsBeingDeleted())
            return;
        WebView::LoadUrl(m_browser, m_page_url);
    });
}

bool SatisfactionSurveyDialog::set_shown_context(std::string context_json)
{
    if (m_browser == nullptr || m_failed || !m_page_ready || IsShown() ||
        m_shown_context_sent || !m_shown_context_json.empty())
        return false;

    try {
        const nlohmann::json context = nlohmann::json::parse(context_json);
        if (!context.is_object())
            return false;
    } catch (const std::exception&) {
        return false;
    }

    m_shown_context_json = std::move(context_json);
    return true;
}

void SatisfactionSurveyDialog::on_show(wxShowEvent& event)
{
    event.Skip();
    if (event.IsShown())
        apply_dialog_size();
    if (!event.IsShown() || m_shown_context_sent || m_browser == nullptr ||
        m_failed || !m_page_ready || m_shown_context_json.empty())
        return;

    m_shown_context_sent = true;
    // Encode the JSON text as a JavaScript string literal instead of directly
    // concatenating object fields into executable source.
    const std::string encoded_context = nlohmann::json(m_shown_context_json).dump();
    m_shown_context_json.clear();
    const std::string script =
        "window.CrealitySurvey.onShown(JSON.parse(" + encoded_context + "));";
    if (!WebView::RunScript(m_browser, wxString::FromUTF8(script.c_str()))) {
        BOOST_LOG_TRIVIAL(error)
            << "[SatisfactionSurvey] failed to notify the page that the dialog is visible";
        m_failed = true;
        CallAfter([this]() {
            if (IsModal())
                EndModal(wxID_ABORT);
        });
    }
}

void SatisfactionSurveyDialog::on_error(wxWebViewEvent& event)
{
    BOOST_LOG_TRIVIAL(warning) << "[SatisfactionSurvey] survey page failed to load, error="
                               << static_cast<int>(event.GetInt());
    signal_failed();
    event.Skip();
}

void SatisfactionSurveyDialog::on_script_message(wxWebViewEvent& event)
{
    if (m_browser == nullptr ||
        !is_satisfaction_survey_url_from_origin(
            m_browser->GetCurrentURL().ToUTF8().data(), m_trusted_origin)) {
        event.Skip();
        return;
    }

    const SurveyMessage message = parse_survey_message(event.GetString());
    if (message.command == "survey_ready") {
        // Receiving this message already proves that the trusted document is
        // executing.  Some WebView backends dispatch script messages just
        // before their host-side LOADED event, so do not impose event ordering.
        if (!m_failed && !m_page_ready) {
            m_page_ready = true;
            if (m_on_ready)
                m_on_ready();
        }
    } else if (message.command == "survey_resize") {
        request_dialog_height(message.requested_height_dip);
    } else if (message.command == "survey_closed") {
        close_from_page(wxID_CANCEL);
    } else if (message.command == "survey_submitted") {
        close_from_page(wxID_OK);
    } else if (message.command == "survey_error") {
        if (IsModal())
            EndModal(wxID_ABORT);
        else
            signal_failed();
    }
    event.Skip();
}

void SatisfactionSurveyDialog::on_navigating(wxWebViewEvent& event)
{
    const std::string url = event.GetURL().ToUTF8().data();
    if (!m_page_ready &&
        (url == "about:blank" ||
         is_satisfaction_survey_url_from_origin(url, m_trusted_origin))) {
        event.Skip();
        return;
    }

    // The dialog is preloaded while hidden.  Never turn page-controlled
    // navigation into an unexpected native browser launch, and freeze the
    // trusted document after its ready handshake.
    event.Veto();
}

void SatisfactionSurveyDialog::on_new_window(wxWebViewEvent& event)
{
    event.Veto();
}

void SatisfactionSurveyDialog::apply_dialog_size()
{
    const wxRect work_area = display_work_area(this);
    const int work_area_margin = FromDIP(kWorkAreaMarginDip);
    const int maximum_width = std::max(1, work_area.GetWidth() - work_area_margin);
    const int maximum_height = std::max(1, work_area.GetHeight() - work_area_margin);
    const int maximum_height_dip = std::max(1, ToDIP(maximum_height));
    const int requested_height_dip = std::min(m_requested_height_dip, maximum_height_dip);

    const int minimum_width = std::min(FromDIP(kMinimumDialogWidthDip), maximum_width);
    const int minimum_height = std::min(FromDIP(kMinimumDialogHeightDip), maximum_height);
    const wxSize minimum_size(minimum_width, minimum_height);
    const wxSize target_size(
        std::clamp(FromDIP(kDialogWidthDip), minimum_width, maximum_width),
        std::clamp(FromDIP(requested_height_dip), minimum_height, maximum_height));

    if (GetMinSize() != minimum_size)
        SetMinSize(minimum_size);

    const bool size_changed = GetSize() != target_size;
    if (size_changed)
        SetSize(target_size);
    Layout();
    if (size_changed && IsShown())
        CentreOnParent();
}

void SatisfactionSurveyDialog::request_dialog_height(int height_dip)
{
    if (height_dip == m_requested_height_dip)
        return;
    m_requested_height_dip = height_dip;
    apply_dialog_size();
}

void SatisfactionSurveyDialog::signal_failed()
{
    if (m_failed)
        return;
    m_failed = true;
    if (IsModal()) {
        EndModal(wxID_ABORT);
        return;
    }
    if (m_on_failed)
        m_on_failed();
}

void SatisfactionSurveyDialog::close_from_page(int return_code)
{
    if (IsModal())
        EndModal(return_code);
    else
        signal_failed();
}

void SatisfactionSurveyDialog::on_dpi_changed(const wxRect& suggested_rect)
{
    (void) suggested_rect;
    apply_dialog_size();
}

} // namespace GUI
} // namespace Slic3r
