#include "ModelDetailDialog.hpp"
#include "WebModelLibraryView.hpp"
#include "GUI.hpp"
#include "I18N.hpp"

#include <wx/sizer.h>
#include <boost/log/trivial.hpp>

namespace Slic3r {
namespace GUI {

namespace {

void apply_reverse_dpi_zoom(wxWindow* window, WebModelLibraryView* view)
{
    wxWebView* browser = view ? view->GetWebView() : nullptr;
    if (!window || !browser || !browser->GetNativeBackend())
        return;

    const double dpi_scale = window->GetDPIScaleFactor();
    const float  page_zoom = dpi_scale > 0.0 ? static_cast<float>(1.0 / dpi_scale) : 1.0f;
    browser->SetZoomFactor(page_zoom);
}

} // namespace

ModelDetailDialog::ModelDetailDialog(wxWindow*       parent,
                                     const wxString& url,
                                     const wxString& title)
    : wxDialog(parent,
               wxID_ANY,
               title.IsEmpty() ? wxString(_L("Model Detail")) : title,
               wxDefaultPosition,
               wxDefaultSize,
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxMAXIMIZE_BOX)
{
    SetSize(wxSize(1100, 780));
    SetBackgroundColour(wxColour(54, 54, 56));

    // Reuse WebModelLibraryView so UA, cookies, CXSWGroupInterface script
    // message handler and F12 dev-tools are all set up identically.
    m_view = new WebModelLibraryView(this);

    auto* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(m_view, 1, wxEXPAND, 0);
    SetSizer(sizer);

    wxWebView* browser = m_view->GetWebView();
    if (browser) {
        Bind(wxEVT_DPI_CHANGED, [this](wxDPIChangedEvent& evt) {
            evt.Skip();
            CallAfter([this]() {
                if (m_load_count > 0)
                    apply_reverse_dpi_zoom(this, m_view);
            });
        });

        // On first page load, WebView2 is fully initialised - re-inject cookies
        // and reload so the auth headers are sent correctly.
        // The dialog remains hidden during this whole cycle thanks to Show() override.
        browser->Bind(wxEVT_WEBVIEW_LOADED, [this](wxWebViewEvent& evt) {
            m_load_count++;
            apply_reverse_dpi_zoom(this, m_view);
            if (!m_cookies_injected) {
                m_cookies_injected = true;
                BOOST_LOG_TRIVIAL(info) << "[ModelDetailDialog] First load complete (count=" << m_load_count
                                        << "), refreshing UA+cookies";
                m_view->UpdateUserAgent();
            } else {
                // Page is now fully loaded with correct cookies & UA.
                // Safe to reveal the dialog.
                BOOST_LOG_TRIVIAL(info) << "[ModelDetailDialog] Page ready after reload (count=" << m_load_count
                                        << "), showing dialog";
                m_page_ready = true;
                Show(true);
            }
            evt.Skip();
        }, browser->GetId());

        // Close the dialog when the user triggers a download.
        // The page sends a CXSWGroupInterface script message with
        // command == "3mf_download_start" at that point.
        browser->Bind(wxEVT_WEBVIEW_SCRIPT_MESSAGE_RECEIVED, [this](wxWebViewEvent& evt) {
            try {
                const std::string msg = evt.GetString().ToUTF8().data();
                if (msg.find("3mf_download_start") != std::string::npos) {
                    BOOST_LOG_TRIVIAL(info) << "[ModelDetailDialog] 3mf_download_start detected, closing dialog";
                    Hide();
                    CallAfter([this]() { Close(); });
                }
            } catch (...) {}
            evt.Skip();
        }, browser->GetId());
    }

    if (!url.IsEmpty()) {
        m_view->load_url(url);
        BOOST_LOG_TRIVIAL(info) << "[ModelDetailDialog] Loading URL: " << url.ToStdString();
    }
}

bool ModelDetailDialog::Show(bool show)
{
    // Block Show(true) until cookies & UA have been fully injected
    // (i.e. the second wxEVT_WEBVIEW_LOADED has fired).  Show(false)
    // (hiding) is always allowed.
    if (show && !m_page_ready) {
        BOOST_LOG_TRIVIAL(info) << "[ModelDetailDialog] Show(true) blocked - page not ready yet";
        return false;
    }
    return wxDialog::Show(show);
}

/* static */
wxString ModelDetailDialog::BuildDetailUrl(const std::string& model_id)
{
    if (model_id.empty())
        return wxEmptyString;

    // Pattern: https://cp.crealitycloud.com/flowprint/slice-software/model-detail/<model_id>
    const std::string base = get_cloud_webaddress();
    return wxString::FromUTF8(
        (base + "flowprint/slice-software/model-detail/" + model_id).c_str());
}

} // namespace GUI
} // namespace Slic3r
