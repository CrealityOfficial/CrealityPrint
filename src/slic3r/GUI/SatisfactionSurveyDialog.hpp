#ifndef slic3r_GUI_SatisfactionSurveyDialog_hpp_
#define slic3r_GUI_SatisfactionSurveyDialog_hpp_

#include "GUI_Utils.hpp"

#include <functional>
#include <string>

class wxWebView;
class wxWebViewEvent;
class wxShowEvent;

namespace Slic3r {
namespace GUI {

// Both the manager and the WebView host use the same exact origin check.  The
// trusted origin is selected once by SatisfactionSurveyManager, so enabling a
// local survey endpoint cannot leave the dialog's navigation guard pointing at
// the production origin (or vice versa).
bool is_satisfaction_survey_url_from_origin(const std::string& url,
                                            const std::string& trusted_origin);

// A deliberately thin host for the server-owned survey page.  The page owns
// form rendering and submission; the desktop client receives lifecycle/layout
// messages only and therefore never handles or logs the user's feedback/contact data.
class SatisfactionSurveyDialog final : public DPIDialog
{
public:
    using Callback = std::function<void()>;

    SatisfactionSurveyDialog(wxWindow* parent,
                             const wxString& page_url,
                             std::string trusted_origin,
                             Callback on_ready,
                             Callback on_failed);
    ~SatisfactionSurveyDialog() override;

    void start_loading();
    // Must be provided before ShowModal().  The context is delivered exactly
    // once from the native show event so hidden WebView preloading is never
    // mistaken for a real survey exposure.
    bool set_shown_context(std::string context_json);
    bool page_ready() const { return m_page_ready; }
    bool load_failed() const { return m_failed; }

protected:
    void on_dpi_changed(const wxRect& suggested_rect) override;

private:
    void on_error(wxWebViewEvent& event);
    void on_show(wxShowEvent& event);
    void on_script_message(wxWebViewEvent& event);
    void on_navigating(wxWebViewEvent& event);
    void on_new_window(wxWebViewEvent& event);
    void apply_dialog_size();
    void request_dialog_height(int height_dip);
    void signal_failed();
    void close_from_page(int return_code);

    wxWebView* m_browser {nullptr};
    wxString   m_page_url;
    std::string m_trusted_origin;
    std::string m_shown_context_json;
    Callback   m_on_ready;
    Callback   m_on_failed;
    bool       m_loading_started {false};
    bool       m_page_ready {false};
    bool       m_shown_context_sent {false};
    bool       m_failed {false};
    int        m_requested_height_dip {680};
};

} // namespace GUI
} // namespace Slic3r

#endif // slic3r_GUI_SatisfactionSurveyDialog_hpp_
