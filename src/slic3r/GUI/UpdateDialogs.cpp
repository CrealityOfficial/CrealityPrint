#include "UpdateDialogs.hpp"
#include "ProfileUpdatePolicy.hpp"
#include <wx/display.h>

#include <cstring>
#include <boost/format.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/event.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/statbmp.h>
#include <wx/checkbox.h>

#include "slic3r/GUI/Auxiliary.hpp"
#include "libslic3r/libslic3r.h"
#include "libslic3r/Utils.hpp"
#include "GUI.hpp"
#include "GUI_App.hpp"
#include "I18N.hpp"
#include "ConfigWizard.hpp"
#include "wxExtensions.hpp"
#include "MainFrame.hpp"
#include "libslic3r/common_header/common_header.h"
namespace Slic3r {
namespace GUI {


PresetUpdateDialog::PresetUpdateDialog(wxWindow *parent, std::function<void()> action, std::function<void(bool)> dismissed)
    : DPIDialog(parent, wxID_ANY, _L("Check for Preset Updates"), wxDefaultPosition, wxDefaultSize,
                wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER), m_action(std::move(action)), m_dismissed(std::move(dismissed))
{
    SetFont(wxGetApp().normal_font());
    // The system dialog grey is not in StateColor's light/dark map.
    SetBackgroundColour(*wxWHITE);
    auto *layout = new wxBoxSizer(wxVERTICAL);
    m_heading = new Label(this, GetFont().Scaled(1.25).Bold(), _L("Checking preset updates..."));
    layout->Add(m_heading, 0, wxEXPAND | wxALL, FromDIP(24));
    m_scroll = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL | wxTAB_TRAVERSAL);
    m_scroll->SetScrollRate(0, FromDIP(12));
    m_scroll->SetMinSize(wxSize(1, 1));
    m_contents = new wxBoxSizer(wxVERTICAL);
    m_scroll->SetSizer(m_contents);
    layout->Add(m_scroll, 1, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(24));
    auto *buttons = new wxBoxSizer(wxHORIZONTAL);
    buttons->AddStretchSpacer();
    m_secondary = new Button(this, _L("Remind me later"));
    m_primary = new Button(this, _L("Immediate Update"));
    m_secondary->SetMinSize(FromDIP(wxSize(110, 32)));
    m_primary->SetMinSize(FromDIP(wxSize(130, 32)));
    buttons->Add(m_secondary, 0, wxRIGHT, FromDIP(12));
    buttons->Add(m_primary);
    layout->Add(buttons, 0, wxEXPAND | wxALL, FromDIP(24));
    SetSizer(layout);

    const int display_index = wxDisplay::GetFromWindow(parent);
    const wxSize available = wxDisplay(display_index == wxNOT_FOUND ? 0u : unsigned(display_index)).GetClientArea().GetSize();
    const wxSize minimum = FromDIP(wxSize(480, 360));
    SetMinSize(wxSize(std::min(minimum.x, available.x), std::min(minimum.y, available.y)));
    const wxSize preferred = FromDIP(wxSize(680, 520));
    SetSize(wxSize(std::min(preferred.x, available.x), std::min(preferred.y, available.y)));
    CenterOnParent();
    m_primary->Bind(wxEVT_BUTTON, [this](wxCommandEvent &) { m_action(); });
    m_secondary->Bind(wxEVT_BUTTON, [this](wxCommandEvent &) {
        if (m_state == State::Available) {
            m_dismissed(true);
            Destroy();
        } else {
            Close();
        }
    });
    Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent &event) {
        if (m_state == State::Updating && event.CanVeto()) { event.Veto(); return; }
        // Title-bar close and Escape dismiss only this prompt.
        m_dismissed(false);
        Destroy();
    });
    Bind(wxEVT_CHAR_HOOK, [this](wxKeyEvent &event) {
        if (event.GetKeyCode() == WXK_ESCAPE) Close();
        else event.Skip();
    });
    Bind(wxEVT_SIZE, [this](wxSizeEvent &event) { wrap_content(); event.Skip(); });
    m_scroll->Bind(wxEVT_SIZE, [this](wxSizeEvent &event) { wrap_content(); event.Skip(); });
    // Preferences updates do not send a system-colour event to every modeless
    // dialog. Refresh this window on the next idle event when the app theme changes.
    Bind(wxEVT_IDLE, [this](wxIdleEvent &event) {
        if (m_dark_theme != wxGetApp().dark_mode()) apply_theme();
        event.Skip();
    });
    set_state(State::Checking, nlohmann::json::array());
}

void PresetUpdateDialog::set_state(State state, const nlohmann::json &packages, const wxString &message)
{
    m_state = state;
    Freeze();
    m_labels.clear();
    m_version_labels.clear();
    m_contents->Clear(true);
    auto add_text = [this](const wxString &text, bool bold, int space) {
        auto *label = new Label(m_scroll, bold ? GetFont().Bold() : GetFont(), text);
        label->SetMinSize(wxSize(1, -1));
        m_contents->Add(label, 0, wxEXPAND | wxBOTTOM | wxRIGHT, FromDIP(space));
        m_labels.push_back(label);
        return label;
    };
    wxString heading = _L("Parameter update");
    if (state == State::Checking) heading = _L("Checking preset updates...");
    else if (state == State::Updating) heading = _L("Updating parameter packages...");
    else if (state == State::Complete) heading = _L("Parameter packages updated");
    m_heading->SetLabel(heading);
    if (!message.empty()) add_text(message, false, 16);
    if (state == State::Available)
        add_text(_L("Updates are available for the following added printers."), false, 16);
    for (const auto &package : packages) {
        add_text(wxString::FromUTF8(ProfileUpdatePolicy::profile_name(package)), true, 6);
        auto *version = add_text(wxString::Format(state == State::Complete ? _L("Updated to %s") : _L("Target version: %s"),
            wxString::FromUTF8(package.value("showVersion", std::string()))), true, 12);
        m_version_labels.push_back(version);
        add_text(_L("Update content:"), false, 4);
        const auto notes = ProfileUpdatePolicy::release_notes(package);
        add_text(notes.empty() ? _L("No update description was provided.") : wxString::FromUTF8(notes), false, 24);
    }
    const bool busy = state == State::Updating || state == State::Checking;
    m_primary->Show(state != State::Checking);
    m_primary->Enable(!busy);
    m_secondary->Show(state != State::Complete && state != State::Info && state != State::Updating);
    m_secondary->Enable(state != State::Updating);
    m_secondary->SetLabel(state == State::Available ? _L("Remind me later") : _L("Close"));
    m_primary->SetLabel(state == State::Available ? (packages.size() == 1 ? _L("Immediate Update") :
        wxString::Format(_L("Update all (%d)"), int(packages.size()))) :
        state == State::Error ? _L("Retry") : state == State::Updating ? _L("Immediate Update") : _L("OK"));
    apply_theme();
    m_wrap_width = -1;
    Layout();
    wrap_content();
    m_scroll->Scroll(0, 0);
    Thaw();
}

void PresetUpdateDialog::apply_theme()
{
    m_dark_theme = wxGetApp().dark_mode();
    // Reset all native surfaces to mapped light colours before conversion. This
    // also covers labels created after the dialog's initial theme pass.
    auto reset = [](wxWindow *window) {
        window->SetBackgroundColour(*wxWHITE);
        window->SetForegroundColour(wxColour("#262E30"));
    };
    reset(this);
    reset(m_scroll);
    reset(m_heading);
    reset(m_primary);
    reset(m_secondary);
    for (auto *label : m_labels) reset(label);
    wxGetApp().UpdateDlgDarkUI(this);

    for (auto *label : m_version_labels)
        label->SetForegroundColour(m_dark_theme ? wxColour("#35DB7D") : wxColour("#007837"));

    // Button uses StateColor for painting, separately from wxWindow colours.
    const StateColor text(std::make_pair(wxColour("#6B6B6A"), int(StateColor::Disabled)),
                          std::make_pair(wxColour("#262E30"), int(StateColor::Normal)));
    m_secondary->SetTextColor(text);
    m_secondary->SetBorderColor(StateColor(wxColour("#ACACAC")));
    m_secondary->SetBackgroundColor(StateColor(
        std::make_pair(wxColour("#F0F0F1"), int(StateColor::Disabled)),
        std::make_pair(wxColour("#CECECE"), int(StateColor::Pressed)),
        std::make_pair(wxColour("#EEEEEE"), int(StateColor::Hovered)),
        std::make_pair(*wxWHITE, int(StateColor::Normal))));
    m_primary->SetBackgroundColor(StateColor(
        std::make_pair(wxColour("#F0F0F1"), int(StateColor::Disabled)),
        std::make_pair(wxColour("#17CC5F"), int(StateColor::Normal))));
    m_primary->SetBorderColor(StateColor(wxColour("#ACACAC")));
    m_primary->SetTextColor(StateColor(
        std::make_pair(wxColour("#6B6B6A"), int(StateColor::Disabled)),
        std::make_pair(wxColour("#07110b"), int(StateColor::Normal))));
    Refresh();
}

void PresetUpdateDialog::on_sys_color_changed()
{
    apply_theme();
}

void PresetUpdateDialog::wrap_content()
{
    if (!m_scroll) return;
    // Reserve scrollbar space even when hidden, avoiding alternating wrap widths.
    const int width = std::max(1, m_scroll->GetSize().x - FromDIP(32));
    if (width == m_wrap_width) return;
    m_wrap_width = width;
    for (auto *label : m_labels) {
        label->SetMinSize(wxDefaultSize);
        label->Wrap(width);
        label->SetMinSize(wxSize(1, label->GetBestSize().y));
    }
    m_heading->Wrap(std::max(1, GetClientSize().x - FromDIP(48)));
    m_scroll->Layout();
    m_scroll->FitInside();
}

void PresetUpdateDialog::on_dpi_changed(const wxRect &suggested_rect)
{
    m_secondary->Rescale();
    m_primary->Rescale();
    SetSize(suggested_rect);
    m_wrap_width = -1;
    wrap_content();
}

static const char* URL_CHANGELOG = "%1%";
static const char* URL_DOWNLOAD = "%1%";
static const char* URL_DEV = "%1%";

static const std::string CONFIG_UPDATE_WIKI_URL("");


// MsgUpdateSlic3r

MsgUpdateSlic3r::MsgUpdateSlic3r(const Semver &ver_current, const Semver &ver_online)
    : MsgDialog(nullptr, wxString::Format(_(L("%s Update")), SLIC3R_APP_FULL_NAME), _(L("A new version is available")))
{
    //TODO: for new dialog with updater
	//const bool dev_version = ver_online.prerelease() != nullptr;

	//auto *versions = new wxFlexGridSizer(2, 0, VERT_SPACING);
	//versions->Add(new wxStaticText(this, wxID_ANY, _(L("Current version:"))));
	//versions->Add(new wxStaticText(this, wxID_ANY, ver_current.to_string()));
	//versions->Add(new wxStaticText(this, wxID_ANY, _(L("New version:"))));
	//versions->Add(new wxStaticText(this, wxID_ANY, ver_online.to_string()));
	//content_sizer->Add(versions);
	//content_sizer->AddSpacer(VERT_SPACING);

	//if (dev_version) {
	//	const std::string url = (boost::format(URL_DEV) % ver_online.to_string()).str();
	//	const wxString url_wx = from_u8(url);
	//	auto *link = new wxHyperlinkCtrl(this, wxID_ANY, _(L("Changelog & Download")), url_wx);
	//	content_sizer->Add(link);
	//} else {
	//	const auto lang_code = wxGetApp().current_language_code_safe().ToStdString();

	//	const std::string url_log = (boost::format(URL_CHANGELOG) % lang_code).str();
	//	const wxString url_log_wx = from_u8(url_log);
	//	auto *link_log = new wxHyperlinkCtrl(this, wxID_ANY, _(L("Open changelog page")), url_log_wx);
	//	link_log->Bind(wxEVT_HYPERLINK, &MsgUpdateSlic3r::on_hyperlink, this);
	//	content_sizer->Add(link_log);

	//	const std::string url_dw = (boost::format(URL_DOWNLOAD) % lang_code).str();
	//	const wxString url_dw_wx = from_u8(url_dw);
	//	auto *link_dw = new wxHyperlinkCtrl(this, wxID_ANY, _(L("Open download page")), url_dw_wx);
	//	link_dw->Bind(wxEVT_HYPERLINK, &MsgUpdateSlic3r::on_hyperlink, this);
	//	content_sizer->Add(link_dw);
	//}

	//content_sizer->AddSpacer(2*VERT_SPACING);

	//cbox = new wxCheckBox(this, wxID_ANY, _(L("Don't notify about new releases any more")));
	//content_sizer->Add(cbox);
	//content_sizer->AddSpacer(VERT_SPACING);

	finalize();
}

MsgUpdateSlic3r::~MsgUpdateSlic3r() {}

void MsgUpdateSlic3r::on_hyperlink(wxHyperlinkEvent& evt)
{
	wxGetApp().open_browser_with_warning_dialog(evt.GetURL());
}

bool MsgUpdateSlic3r::disable_version_check() const
{
    //TODO: for new dialog with updaterpre
	//return cbox->GetValue();
	return true;
}

// MsgUpdateConfig

MsgUpdateConfig::MsgUpdateConfig(const std::vector<Update> &updates, bool force_before_wizard /* = false*/)
    : DPIDialog(wxGetApp().mainframe, wxID_ANY, _L("Configuration update"), wxDefaultPosition, wxDefaultSize, wxCAPTION)
{
	auto  title = force_before_wizard ? _L("Configuration update") : _L("Configuration update");
	SetTitle(title);

	std::string icon_path = (boost::format("%1%/images/%2%.ico") % resources_dir() % Slic3r::CxBuildInfo::getIconName()).str();
    SetIcon(wxIcon(encode_path(icon_path.c_str()), wxBITMAP_TYPE_ICO));

    SetBackgroundColour(*wxWHITE);
    wxBoxSizer *m_sizer_main = new wxBoxSizer(wxVERTICAL);
    auto        m_line_top   = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 1));
    m_line_top->SetBackgroundColour(wxColour(166, 169, 170));
    m_sizer_main->Add(m_line_top, 0, wxEXPAND, 0);
    m_sizer_main->Add(0, 0, 0, wxTOP, FromDIP(30));

    wxBoxSizer *m_sizer_body = new wxBoxSizer(wxHORIZONTAL);

    m_sizer_body->Add(0, 0, 0, wxLEFT, FromDIP(38));

    auto sm    = create_scaled_bitmap("CrealityPrint", nullptr, 70);
    auto brand = new wxStaticBitmap(this, wxID_ANY, sm, wxDefaultPosition, wxSize(FromDIP(70), FromDIP(70)));

    m_sizer_body->Add(brand, 0, wxALL, 0);

    m_sizer_body->Add(0, 0, 0, wxRIGHT, FromDIP(25));

    wxBoxSizer *m_sizer_right = new wxBoxSizer(wxVERTICAL);


    auto m_text_up_info = new wxStaticText(this, wxID_ANY, _L("A new configuration package available, Do you want to install it?"), wxDefaultPosition, wxDefaultSize, 0);
    m_text_up_info->SetFont(::Label::Head_14);
    m_text_up_info->SetForegroundColour(wxColour(0x26, 0x2E, 0x30));
    m_text_up_info->Wrap(-1);
    m_sizer_right->Add(m_text_up_info, 0, 0, 0);

    m_sizer_right->Add(0, 0, 1, wxTOP, FromDIP(15));

    auto m_scrollwindw_release_note = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxSize(FromDIP(560), FromDIP(430)),wxVSCROLL);
    m_scrollwindw_release_note->SetScrollRate(0, 5);
    m_scrollwindw_release_note->SetBackgroundColour(wxColour(0xF8, 0xF8, 0xF8));
    m_scrollwindw_release_note->SetMaxSize(wxSize(FromDIP(560), FromDIP(430)));
    m_scrollwindw_release_note->SetWindowStyle(wxVSCROLL);

	auto sizer_button = new wxBoxSizer(wxHORIZONTAL);
    sizer_button->Add(0, 0, 1, wxEXPAND, 5);


	StateColor btn_bg_green(std::pair<wxColour, int>(wxColour(0, 137, 123), StateColor::Pressed),
                            std::pair<wxColour, int>(wxColour(38, 166, 154), StateColor::Hovered), std::pair<wxColour, int>(AMS_CONTROL_BRAND_COLOUR, StateColor::Normal));

    StateColor btn_bg_white(std::pair<wxColour, int>(wxColour(206, 206, 206), StateColor::Pressed),
                            std::pair<wxColour, int>(wxColour(238, 238, 238), StateColor::Hovered),
                            std::pair<wxColour, int>(*wxWHITE, StateColor::Normal));

  
	auto m_butto_ok = new Button(this, _L("OK"));
    m_butto_ok->SetBackgroundColor(btn_bg_green);
    m_butto_ok->SetBorderColor(*wxWHITE);
    m_butto_ok->SetTextColor(*wxWHITE);
    m_butto_ok->SetFont(Label::Body_12);
    m_butto_ok->SetSize(wxSize(FromDIP(58), FromDIP(24)));
    m_butto_ok->SetMinSize(wxSize(FromDIP(58), FromDIP(24)));


    auto m_button_cancel = new Button(this, _L("Cancel"));
    m_button_cancel->SetBackgroundColor(*wxWHITE);
    m_button_cancel->SetBorderColor(wxColour(38, 46, 48));
    m_button_cancel->SetFont(Label::Body_12);
    m_button_cancel->SetSize(wxSize(FromDIP(58), FromDIP(24)));
    m_button_cancel->SetMinSize(wxSize(FromDIP(58), FromDIP(24)));


    sizer_button->Add(m_butto_ok, 0, wxALL, 5);
    sizer_button->Add(m_button_cancel, 0, wxALL, 5);

	m_sizer_right->Add(m_scrollwindw_release_note, 0, wxEXPAND | wxRIGHT, FromDIP(20));
    m_sizer_right->Add(sizer_button, 0, wxEXPAND | wxRIGHT, FromDIP(20));

    
    m_sizer_body->Add(m_sizer_right, 1, wxBOTTOM | wxEXPAND, FromDIP(30));
    m_sizer_main->Add(m_sizer_body, 0, wxEXPAND, 0);

	wxBoxSizer *content_sizer             = new wxBoxSizer(wxVERTICAL);

   



	const auto lang_code = wxGetApp().current_language_code_safe().ToStdString();

    auto *versions = new wxBoxSizer(wxVERTICAL);
    // BBS: use changelog string instead of url
    wxStaticText *changelog_textctrl = new wxStaticText(m_scrollwindw_release_note, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(FromDIP(560), -1));


    for (const auto &update : updates) {
        auto *flex = new wxFlexGridSizer(2, 0, FromDIP(15));

        auto *text_vendor = new wxStaticText(m_scrollwindw_release_note, wxID_ANY, update.vendor);
        text_vendor->SetFont(::Label::Body_13);
        flex->Add(text_vendor);
        flex->Add(new wxStaticText(m_scrollwindw_release_note, wxID_ANY, update.version.to_string()));

        // BBS: use changelog string instead of url
        if (!update.comment.empty()) {
            flex->Add(new wxStaticText(m_scrollwindw_release_note, wxID_ANY, _(L("Description:"))), 0, wxALIGN_RIGHT);
            auto *update_comment = new Label(m_scrollwindw_release_note,std::string(""));
            update_comment->SetLabel(from_u8(update.comment));
            update_comment->SetMaxSize(wxSize(FromDIP(545), -1));
            update_comment->SetMinSize(wxSize(FromDIP(545), -1));
            update_comment->Wrap(FromDIP(450));
            flex->Add(update_comment);
        }

        versions->Add(flex);

		

        // BBS: use changelog string instead of url

			//auto change_log = new wxStaticText(m_scrollwindw_release_note, wxID_ANY, from_u8(update.change_log), wxDefaultPosition, wxDefaultSize); 
			changelog_textctrl->SetLabel(changelog_textctrl->GetLabel() + wxString::Format("%s\n", from_u8(update.change_log)));
    }

	content_sizer->Add(versions);

	

    ////BBS: use changelog string instead of url
    if (changelog_textctrl) 
		content_sizer->Add(changelog_textctrl, 1, wxEXPAND | wxTOP, FromDIP(30));


	m_butto_ok->Bind(wxEVT_BUTTON, [this](const wxCommandEvent &) { EndModal(wxID_OK); });
	m_button_cancel->Bind(wxEVT_BUTTON, [this](const wxCommandEvent &) { EndModal(wxID_CLOSE); });


    m_scrollwindw_release_note->SetSizer(content_sizer);
    m_scrollwindw_release_note->Layout();


    SetSizer(m_sizer_main);
    Layout();
    m_sizer_main->Fit(this);

    Centre(wxBOTH);
	wxGetApp().UpdateDlgDarkUI(this);
}

void MsgUpdateConfig::on_dpi_changed(const wxRect &suggested_rect) {}


MsgUpdateConfig::~MsgUpdateConfig() {}

//MsgUpdateForced

MsgUpdateForced::MsgUpdateForced(const std::vector<Update>& updates) :
    MsgDialog(nullptr, _(L("Configuration incompatible")), _(L("the configuration package is incompatible with current application.")) + " ", wxOK | wxICON_ERROR)
{
	auto* text = new wxStaticText(this, wxID_ANY, wxString::Format(_(L(
		"The configuration package is incompatible with current application.\n"
		"%s will update the configuration package, Otherwise it won't be able to start"
	)), SLIC3R_APP_FULL_NAME));
	

	text->Wrap(CONTENT_WIDTH * wxGetApp().em_unit());
	content_sizer->Add(text);
	content_sizer->AddSpacer(VERT_SPACING);

	const auto lang_code = wxGetApp().current_language_code_safe().ToStdString();

	auto* versions = new wxFlexGridSizer(2, 0, VERT_SPACING);
	//BBS: use changelog string instead of url
	wxTextCtrl* changelog_textctrl = nullptr;
	for (const auto& update : updates) {
		auto* text_vendor = new wxStaticText(this, wxID_ANY, update.vendor);
		text_vendor->SetFont(boldfont);
		versions->Add(text_vendor);
		versions->Add(new wxStaticText(this, wxID_ANY, update.version.to_string()));

		//BBS: use changelog string instead of url
		if (!update.comment.empty()) {
			versions->Add(new wxStaticText(this, wxID_ANY, _(L("Description:")))/*, 0, wxALIGN_RIGHT*/);//uncoment if align to right (might not look good if 1  vedor name is longer than other names)
			auto* update_comment = new wxStaticText(this, wxID_ANY, from_u8(update.comment));
			update_comment->Wrap(CONTENT_WIDTH * wxGetApp().em_unit());
			versions->Add(update_comment);
		}
		//BBS: use changelog string instead of url
		if (! update.change_log.empty()) {
			if (!changelog_textctrl)
				changelog_textctrl = new wxTextCtrl(this, wxID_ANY, from_u8(update.change_log), wxDefaultPosition,  wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY|wxHSCROLL);
			else
				changelog_textctrl->AppendText(from_u8(update.change_log));
		}

		//if (!update.changelog_url.empty() && update.version.prerelease() == nullptr) {
		//	auto* line = new wxBoxSizer(wxHORIZONTAL);
		//	auto changelog_url = (boost::format(update.changelog_url) % lang_code).str();
		//	line->AddSpacer(3 * VERT_SPACING);
		//	line->Add(new wxHyperlinkCtrl(this, wxID_ANY, _(L("Open changelog page")), changelog_url));
		//	versions->Add(line);
		//}
	}

	content_sizer->Add(versions);
	//BBS: use changelog string instead of url
	//content_sizer->AddSpacer(2 * VERT_SPACING);
	if (changelog_textctrl)
		content_sizer->Add(changelog_textctrl);

	add_button(wxID_EXIT, false, wxString::Format(_L("Exit %s"), SLIC3R_APP_FULL_NAME));
	for (auto ID : { wxID_EXIT, wxID_OK })
		get_button(ID)->Bind(wxEVT_BUTTON, [this](const wxCommandEvent& evt) { EndModal(evt.GetId()); });

	finalize();
}

MsgUpdateForced::~MsgUpdateForced() {}

// MsgDataIncompatible

MsgDataIncompatible::MsgDataIncompatible(const std::unordered_map<std::string, wxString> &incompats) :
    MsgDialog(nullptr,  _(L("Configuration incompatible")), _(L("the Configuration package is incompatible with current APP.")), wxICON_ERROR)
{
    //TODO
	//auto *text = new wxStaticText(this, wxID_ANY, wxString::Format(_(L(
	//	"This version of %s is not compatible with currently installed configuration bundles.\n"
	//	"This probably happened as a result of running an older %s after using a newer one.\n\n"

	//	"You may either exit %s and try again with a newer version, or you may re-run the initial configuration. "
	//	"Doing so will create a backup snapshot of the existing configuration before installing files compatible with this %s.")) + "\n", 
	//	SLIC3R_APP_NAME, SLIC3R_APP_NAME, SLIC3R_APP_NAME, SLIC3R_APP_NAME));
	//text->Wrap(CONTENT_WIDTH * wxGetApp().em_unit());
	//content_sizer->Add(text);

	//auto *text2 = new wxStaticText(this, wxID_ANY, wxString::Format(_(L("This %s version: %s")), SLIC3R_APP_NAME, SLIC3R_VERSION));
	//text2->Wrap(CONTENT_WIDTH * wxGetApp().em_unit());
	//content_sizer->Add(text2);
	//content_sizer->AddSpacer(VERT_SPACING);

	//auto *text3 = new wxStaticText(this, wxID_ANY, _(L("Incompatible bundles:")));
	//text3->Wrap(CONTENT_WIDTH * wxGetApp().em_unit());
	//content_sizer->Add(text3);
	//content_sizer->AddSpacer(VERT_SPACING);

	//auto *versions = new wxFlexGridSizer(2, 0, VERT_SPACING);
	//for (const auto &incompat : incompats) {
	//	auto *text_vendor = new wxStaticText(this, wxID_ANY, incompat.first);
	//	text_vendor->SetFont(boldfont);
	//	versions->Add(text_vendor);
	//	versions->Add(new wxStaticText(this, wxID_ANY, incompat.second));
	//}

	//content_sizer->Add(versions);
	//content_sizer->AddSpacer(2*VERT_SPACING);

	//add_button(wxID_REPLACE, true, _L("Re-configure"));
	//add_button(wxID_EXIT, false, wxString::Format(_L("Exit %s"), SLIC3R_APP_NAME));

	//for (auto ID : {wxID_EXIT, wxID_REPLACE})
	//	get_button(ID)->Bind(wxEVT_BUTTON, [this](const wxCommandEvent& evt) { this->EndModal(evt.GetId()); });

	finalize();
}

MsgDataIncompatible::~MsgDataIncompatible() {}


// MsgDataLegacy

//MsgDataLegacy::MsgDataLegacy() :
//	MsgDialog(nullptr, _(L("Configuration update")), _(L("Configuration update")))
//{
//    auto *text = new wxStaticText(this, wxID_ANY, from_u8((boost::format(
//        _utf8(L(
//			"%s now uses an updated configuration structure.\n\n"
//
//			"So called 'System presets' have been introduced, which hold the built-in default settings for various "
//			"printers. These System presets cannot be modified, instead, users now may create their "
//			"own presets inheriting settings from one of the System presets.\n"
//			"An inheriting preset may either inherit a particular value from its parent or override it with a customized value.\n\n"
//
//			"Please proceed with the %s that follows to set up the new presets "
//			"and to choose whether to enable automatic preset updates."
//        )))
//        % SLIC3R_APP_NAME
//        % _utf8(ConfigWizard::name())).str()
//	));
//	text->Wrap(CONTENT_WIDTH * wxGetApp().em_unit());
//	content_sizer->Add(text);
//	content_sizer->AddSpacer(VERT_SPACING);
//
//	auto *text2 = new wxStaticText(this, wxID_ANY, _(L("For more information please visit our wiki page:")));
//	static const wxString url("");
//	// The wiki page name is intentionally not localized:
//	auto *link = new wxHyperlinkCtrl(this, wxID_ANY, wxString::Format("%s 1.40 configuration update", SLIC3R_APP_NAME), CONFIG_UPDATE_WIKI_URL);
//	content_sizer->Add(text2);
//	content_sizer->Add(link);
//	content_sizer->AddSpacer(VERT_SPACING);
//
//	finalize();
//}
//
//MsgDataLegacy::~MsgDataLegacy() {}

// MsgNoUpdate

MsgNoUpdates::MsgNoUpdates() :
    MsgDialog(nullptr, _(L("Configuration updates")), _(L("No updates available.")), wxICON_ERROR | wxOK)
{

	auto* text = new wxStaticText(this, wxID_ANY, _(L("The configuration is up to date.")));
	text->Wrap(CONTENT_WIDTH * wxGetApp().em_unit());
	content_sizer->Add(text);
	content_sizer->AddSpacer(VERT_SPACING);

	finalize();
}

MsgNoUpdates::~MsgNoUpdates() {}

}
}
