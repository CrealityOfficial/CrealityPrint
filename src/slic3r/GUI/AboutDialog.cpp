#include "AboutDialog.hpp"
#include "I18N.hpp"

#include "libslic3r/Utils.hpp"
#include "libslic3r/Color.hpp"
#include "GUI.hpp"
#include "GUI_App.hpp"
#include "MainFrame.hpp"
#include "format.hpp"
#include "Widgets/Button.hpp"

#include <string>
#include <wx/clipbrd.h>
#include <wx/gdicmn.h>
#include <wx/string.h>

namespace Slic3r {
namespace GUI {

AboutDialogLogo::AboutDialogLogo(wxWindow* parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize)
{
    this->SetBackgroundColour(*wxWHITE);
    this->logo = ScalableBitmap(this, Slic3r::var("CrealityPrint_192px.png"), wxBITMAP_TYPE_PNG);
    this->SetMinSize(this->logo.GetBmpSize());

    this->Bind(wxEVT_PAINT, &AboutDialogLogo::onRepaint, this);
}

void AboutDialogLogo::onRepaint(wxEvent &event)
{
    wxPaintDC dc(this);
    dc.SetBackgroundMode(wxTRANSPARENT);

    wxSize size = this->GetSize();
    int logo_w = this->logo.GetBmpWidth();
    int logo_h = this->logo.GetBmpHeight();
    dc.DrawBitmap(this->logo.bmp(), (size.GetWidth() - logo_w)/2, (size.GetHeight() - logo_h)/2, true);

    event.Skip();
}


// -----------------------------------------
// CopyrightsDialog
// -----------------------------------------
CopyrightsDialog::CopyrightsDialog()
    : DPIDialog(static_cast<wxWindow*>(wxGetApp().mainframe), wxID_ANY, from_u8((boost::format("%1% - %2%")
        % (wxGetApp().is_editor() ? SLIC3R_APP_FULL_NAME : GCODEVIEWER_APP_NAME)
        % _utf8(L("Portions copyright"))).str()),
        wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    this->SetFont(wxGetApp().normal_font());
	this->SetBackgroundColour(*wxWHITE);

    std::string icon_path = (boost::format("%1%/images/Creative3DTitle.ico") % resources_dir()).str();
    SetIcon(wxIcon(encode_path(icon_path.c_str()), wxBITMAP_TYPE_ICO));

    wxStaticLine *staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );

	auto sizer = new wxBoxSizer(wxVERTICAL);
    //sizer->Add( staticline1, 0, wxEXPAND | wxALL, 5 );

    fill_entries();

    m_html = new wxHtmlWindow(this, wxID_ANY, wxDefaultPosition,
                              wxSize(40 * em_unit(), 20 * em_unit()), wxHW_SCROLLBAR_AUTO);
    m_html->SetMinSize(wxSize(FromDIP(870),FromDIP(520)));
    m_html->SetBackgroundColour(*wxWHITE);
    wxFont font = get_default_font(this);
    const int fs = font.GetPointSize();
    const int fs2 = static_cast<int>(1.2f*fs);
    int size[] = { fs, fs, fs, fs, fs2, fs2, fs2 };

    m_html->SetFonts(font.GetFaceName(), font.GetFaceName(), size);
    m_html->SetBorders(2);
    m_html->SetPage(get_html_text());

    sizer->Add(m_html, 1, wxEXPAND | wxALL, 15);
    m_html->Bind(wxEVT_HTML_LINK_CLICKED, &CopyrightsDialog::onLinkClicked, this);

    SetSizer(sizer);
    sizer->SetSizeHints(this);
    CenterOnParent();
    wxGetApp().UpdateDlgDarkUI(this);
}

void CopyrightsDialog::fill_entries()
{
    m_entries = {
        { "Admesh",                                         "",      "https://admesh.readthedocs.io/" },
        { "Anti-Grain Geometry",                            "",      "http://antigrain.com" },
        { "ArcWelderLib",                                   "",      "https://plugins.octoprint.org/plugins/arc_welder" },
        { "Boost",                                          "",      "http://www.boost.org" },
        { "Cereal",                                         "",      "http://uscilab.github.io/cereal" },
        { "CGAL",                                           "",      "https://www.cgal.org" },
        { "Clipper",                                        "",      "http://www.angusj.co" },
        { "libcurl",                                        "",      "https://curl.se/libcurl" },
        { "Eigen3",                                         "",      "http://eigen.tuxfamily.org" },
        { "Expat",                                          "",      "http://www.libexpat.org" },
        { "fast_float",                                     "",      "https://github.com/fastfloat/fast_float" },
        { "GLEW (The OpenGL Extension Wrangler Library)",   "",      "http://glew.sourceforge.net" },
        { "GLFW",                                           "",      "https://www.glfw.org" },
        { "GNU gettext",                                    "",      "https://www.gnu.org/software/gettext" },
        { "ImGUI",                                          "",      "https://github.com/ocornut/imgui" },
        { "ImGuizmo",                                       "",      "https://github.com/CedricGuillemet/ImGuizmo" },
        { "Libigl",                                         "",      "https://libigl.github.io" },
        { "libnest2d",                                      "",      "https://github.com/tamasmeszaros/libnest2d" },
        { "lib_fts",                                        "",      "https://www.forrestthewoods.com" },
        { "Mesa 3D",                                        "",      "https://mesa3d.org" },
        { "Miniz",                                          "",      "https://github.com/richgel999/miniz" },
        { "Nanosvg",                                        "",      "https://github.com/memononen/nanosvg" },
        { "nlohmann/json",                                  "",      "https://json.nlohmann.me" },
        { "Qhull",                                          "",      "http://qhull.org" },
        { "Open Cascade",                                   "",      "https://www.opencascade.com" },
        { "OpenGL",                                         "",      "https://www.opengl.org" },
        { "PoEdit",                                         "",      "https://poedit.net" },
        { "PrusaSlicer",                                    "",      "https://www.prusa3d.com" },
        { "Real-Time DXT1/DXT5 C compression library",      "",      "https://github.com/Cyan4973/RygsDXTc" },
        { "SemVer",                                         "",      "https://semver.org" },
        { "Shinyprofiler",                                  "",      "https://code.google.com/p/shinyprofiler" },
        { "SuperSlicer",                                    "",      "https://github.com/supermerill/SuperSlicer" },
        { "TBB",                                            "",      "https://www.intel.cn/content/www/cn/zh/developer/tools/oneapi/onetbb.html" },
        { "wxWidgets",                                      "",      "https://www.wxwidgets.org" },
        { "zlib",                                           "",      "http://zlib.net" },

    };
}

wxString CopyrightsDialog::get_html_text()
{
    wxColour bgr_clr = wxGetApp().get_window_default_clr();//wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);

    const auto text_clr = wxGetApp().get_label_clr_default();// wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
    const auto text_clr_str = encode_color(ColorRGB(text_clr.Red(), text_clr.Green(), text_clr.Blue()));
    const auto bgr_clr_str = encode_color(ColorRGB(bgr_clr.Red(), bgr_clr.Green(), bgr_clr.Blue()));

    const wxString copyright_str = _L("Copyright") + "&copy; ";

    wxString text = wxString::Format(
        "<html>"
            "<body bgcolor= %s link= %s>"
            "<font color=%s>"
                "<font size=\"5\">%s</font><br/>"
                "<font size=\"5\">%s</font>"
                "<a href=\"%s\">%s.</a><br/>"
                "<font size=\"5\">%s.</font><br/>"
                "<br /><br />"
                "<font size=\"5\">%s</font><br/>"
                "<font size=\"5\">%s:</font><br/>"
                "<br />"
                "<font size=\"3\">",
         bgr_clr_str, text_clr_str, text_clr_str,
        _L("License"),
        _L("CrealityPrint is licensed under "),
        "https://www.gnu.org/licenses/agpl-3.0.html",_L("GNU Affero General Public License, version 3"),
        _L("CrealityPrint is based on PrusaSlicer and BambuStudio"),
        _L("Libraries"),
        _L("This software uses open source components whose copyright and other proprietary rights belong to their respective owners"));

    for (auto& entry : m_entries) {
        text += format_wxstr(
                    "%s<br/>"
                    , entry.lib_name);

         text += wxString::Format(
                    "<a href=\"%s\">%s</a><br/><br/>"
                    , entry.link, entry.link);
    }

    text += wxString(
                "</font>"
            "</font>"
            "</body>"
        "</html>");

    return text;
}

void CopyrightsDialog::on_dpi_changed(const wxRect &suggested_rect)
{
    const wxFont& font = GetFont();
    const int fs = font.GetPointSize();
    const int fs2 = static_cast<int>(1.2f*fs);
    int font_size[] = { fs, fs, fs, fs, fs2, fs2, fs2 };

    m_html->SetFonts(font.GetFaceName(), font.GetFaceName(), font_size);

    const int& em = em_unit();

    msw_buttons_rescale(this, em, { wxID_CLOSE });

    const wxSize& size = wxSize(40 * em, 20 * em);

    m_html->SetMinSize(size);
    m_html->Refresh();

    SetMinSize(size);
    Fit();

    Refresh();
}

void CopyrightsDialog::onLinkClicked(wxHtmlLinkEvent &event)
{
    wxGetApp().open_browser_with_warning_dialog(event.GetLinkInfo().GetHref());
    event.Skip(false);
}

void CopyrightsDialog::onCloseDialog(wxEvent &)
{
     this->EndModal(wxID_CLOSE);
}

AboutDialog::AboutDialog()
    : DPIDialog(static_cast<wxWindow *>(wxGetApp().mainframe),wxID_ANY,_L("About Us"),wxDefaultPosition,
        wxDefaultSize, /*wxCAPTION*/wxDEFAULT_DIALOG_STYLE)
{
    SetFont(wxGetApp().normal_font());
	SetBackgroundColour(*wxWHITE);

    std::string icon_path = (boost::format("%1%/images/Creative3DTitle.ico") % resources_dir()).str();
    SetIcon(wxIcon(encode_path(icon_path.c_str()), wxBITMAP_TYPE_ICO));

    // wxPanel *m_panel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(FromDIP(560), FromDIP(237)), wxTAB_TRAVERSAL);
    wxPanel *m_panel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(FromDIP(800), FromDIP(200)), wxTAB_TRAVERSAL);

    wxBoxSizer *panel_versizer = new wxBoxSizer(wxVERTICAL);

    m_panel->SetSizer(panel_versizer);

    wxBoxSizer *ver_sizer = new wxBoxSizer(wxVERTICAL);

	auto main_sizer = new wxBoxSizer(wxVERTICAL);
    main_sizer->Add(m_panel, 0, wxEXPAND | wxALL, 0);
    main_sizer->Add(ver_sizer, 0, wxEXPAND | wxALL, 0);

    const wxColour font_bg = wxGetApp().dark_mode() ? wxColour("#4B4B4D") : wxColour("#FFFFFF");
    //const wxColour font_text_clr = wxGetApp().dark_mode() ?  wxColour("#FFFFFF") : wxColour("#000000");
    bool is_zh = wxGetApp().app_config->get("language") == "zh_CN";

    // logo and creality_name text
    m_logo_bitmap = ScalableBitmap(this, "Creality3D_about", 32);
    m_logo = new wxStaticBitmap(this, wxID_ANY, m_logo_bitmap.bmp(), wxDefaultPosition,wxDefaultSize, 0);
    // m_logo->SetSizer(vesizer);

    // vesizer->Add(0, FromDIP(80), 1, wxEXPAND, FromDIP(5));
    std::string   name_text     = "Creality Print";
    wxStaticText* creality_name = new wxStaticText(this, wxID_ANY, name_text.c_str(), wxDefaultPosition, wxDefaultSize);
    creality_name->SetFont(Label::Body_14);
    creality_name->SetBackgroundColour(font_bg);
    // vesizer->Add(creality_name, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, FromDIP(5));

    wxBoxSizer* top_sizer = new wxBoxSizer(wxHORIZONTAL);
    // Add horizontal spacer to move m_logo to the right
    top_sizer->AddSpacer(FromDIP(30));
    top_sizer->Add(m_logo, 0, wxALL | wxALIGN_LEFT, FromDIP(5));
    top_sizer->Add(creality_name, 0, wxALL | wxALIGN_CENTER_VERTICAL, FromDIP(5));

    // Add vertical spacer to move down m_logo a little bit
    panel_versizer->AddSpacer(FromDIP(18));
    panel_versizer->Add(top_sizer, 0, wxALL | wxEXPAND, 0);

    panel_versizer->AddSpacer(is_zh ? FromDIP(35) : FromDIP(15) );

    //official_introduction
    {
        wxString   official_introduction     = _L("official_introduction");
        wxStaticText* official_introduction_text = new wxStaticText(this, wxID_ANY, official_introduction, wxDefaultPosition, wxDefaultSize);
        official_introduction_text->SetFont(Label::Body_12);
        official_introduction_text->SetBackgroundColour(font_bg);
        panel_versizer->Add(official_introduction_text, 0, wxALL | wxALIGN_LEFT, is_zh ? FromDIP(10) : FromDIP(10));
    }

    panel_versizer->AddStretchSpacer(is_zh ? FromDIP(30) : FromDIP(10));

    // version
    {
        auto version_string = _L("Creality Print ") + " V" + std::string(CREALITYPRINT_VERSION) + " " + get_vertion_type();
        wxStaticText* version_text = new wxStaticText(this, wxID_ANY, version_string.c_str(), wxDefaultPosition, wxDefaultSize);
        version_text->SetFont(Label::Body_12);
        version_text->SetBackgroundColour(font_bg);
        version_text->SetSize(-1,50);

        panel_versizer->Add(version_text, 0, wxALL | wxALIGN_LEFT, FromDIP(10));
    }

    panel_versizer->AddSpacer(FromDIP(2));

    wxBoxSizer *text_sizer_horiz = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer *text_sizer = new wxBoxSizer(wxVERTICAL);
    text_sizer_horiz->Add( 0, 0, 0, wxLEFT, FromDIP(0));

    auto version_string = _L("Creality Print ") + " V" + std::string(CREALITYPRINT_VERSION);
    std::vector<wxString> text_list;
    wxString pre_empty_space = "    ";
    text_list.push_back(pre_empty_space + version_string + _L("engine_copyright1"));
    text_list.push_back(pre_empty_space + _L("engine_copyright2"));
    text_list.push_back(pre_empty_space + _L("engine_copyright3"));
    text_list.push_back(pre_empty_space + _L("engine_copyright4"));

    text_sizer->Add( 0, 0, 0, wxTOP, FromDIP(33));
    
    for (int i = 0; i < text_list.size(); i++)
    {
        auto staticText = new wxStaticText( this, wxID_ANY, wxEmptyString,wxDefaultPosition,wxSize(FromDIP(650), -1), wxALIGN_LEFT );
        staticText->SetBackgroundColour(font_bg);
        staticText->SetMinSize(wxSize(FromDIP(640), -1));
        staticText->SetFont(Label::Body_12);
        if (is_zh) {
            wxString find_txt = "";
            wxString count_txt = "";
            for (auto  o = 0; o < text_list[i].length(); o++) {
                auto size = staticText->GetTextExtent(count_txt);
                if (size.x < FromDIP(640)) {
                    find_txt += text_list[i][o];
                    count_txt += text_list[i][o];
                } else {
                    find_txt += std::string("\n") + text_list[i][o];
                    count_txt = text_list[i][o];
                }
            }
            staticText->SetLabel(find_txt);
        } else {
            staticText->SetLabel(text_list[i]);
            staticText->Wrap(FromDIP(580));
        }

        text_sizer->Add( staticText, 0, wxUP | wxDOWN, FromDIP(3));
    }

    wxBoxSizer *info_sizer  = new wxBoxSizer(wxHORIZONTAL);

    info_sizer->AddSpacer(FromDIP(15));
    wxStaticText *website_text = new wxStaticText(this, wxID_ANY, _L("aboutusdialog_webside"), wxDefaultPosition, wxDefaultSize);
    website_text->SetBackgroundColour(font_bg);
    info_sizer->Add(website_text, 0, wxALL | wxALIGN_LEFT , 0);
    m_website_html = new wxHtmlWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHW_SCROLLBAR_NEVER /*NEVER*/);
    {
          wxFont font = get_default_font(this);
          const int fs = font.GetPointSize()-1;
          int size[] = {fs,fs,fs,fs,fs,fs,fs};
          m_website_html->SetFonts(font.GetFaceName(), font.GetFaceName(), size);
          m_website_html->SetMinSize(wxSize(FromDIP(-1), FromDIP(16)));
          m_website_html->SetBorders(2);
          wxString website_url = _L("offical_webside");
          const auto text = from_u8(
              (boost::format(
              "<html>"
              "<body>"
              "<p style=\"text-align:left\"><a  href=\"%1%\">%1%</ a></p>"
              "</body>"
              "</html>")
            % website_url).str());
          m_website_html->SetPage(text);
          info_sizer->Add(m_website_html, 1, wxALL | wxALIGN_LEFT, 0);
          m_website_html->Bind(wxEVT_HTML_LINK_CLICKED, &AboutDialog::onLinkClicked, this);
      }

    info_sizer->AddSpacer(FromDIP(5));
    wxStaticText *email_text = new wxStaticText(this, wxID_ANY, _L("aboutusdialog_email"), wxDefaultPosition, wxDefaultSize);
    email_text->SetBackgroundColour(font_bg);
    info_sizer->Add(email_text, 0, wxLEFT | wxALIGN_LEFT , 0);
    info_sizer->AddSpacer(FromDIP(5));
    m_email_html = new wxHtmlWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHW_SCROLLBAR_NEVER /*NEVER*/);
    {
          wxFont font = get_default_font(this);
          const int fs = font.GetPointSize()-1;
          int size[] = {fs,fs,fs,fs,fs,fs,fs};
          m_email_html->SetFonts(font.GetFaceName(), font.GetFaceName(), size);
          m_email_html->SetMinSize(wxSize(FromDIP(-1), FromDIP(16)));
          m_email_html->SetBorders(2);
          wxString email_url = _L("offical_email");
          const auto text = from_u8(
              (boost::format(
              "<html>"
              "<body>"
              "<p style=\"text-align:left\"><a  href=\"%1%\">%1%</ a></p>"
              "</body>"
              "</html>")
            % email_url).str());
          m_email_html->SetPage(text);
          info_sizer->Add(m_email_html, 1, wxLEFT |wxALIGN_LEFT, 0);
          m_email_html->Bind(wxEVT_HTML_LINK_CLICKED, &AboutDialog::onLinkClicked, this);
      }

    info_sizer->AddSpacer(FromDIP(5));

    wxStaticText *telephone_text = new wxStaticText(this, wxID_ANY, _L("aboutusdialog_telephone"), wxDefaultPosition, wxDefaultSize);
    telephone_text->SetBackgroundColour(font_bg);
    info_sizer->Add(telephone_text, 0, wxALL | wxALIGN_LEFT , 0);

    info_sizer->AddSpacer(FromDIP(5));

    wxStaticText *telephone_number_text = new wxStaticText(this, wxID_ANY, _L("offical_telephone"), wxDefaultPosition, wxDefaultSize);
    telephone_number_text->SetBackgroundColour(font_bg);
    info_sizer->Add(telephone_number_text, 0, wxALL | wxALIGN_LEFT , 0);

    info_sizer->AddSpacer(is_zh ? FromDIP(200) : FromDIP(130));

    text_sizer_horiz->Add(text_sizer, 1, wxALL,0);
    ver_sizer->Add(text_sizer_horiz, 0, wxALL,0);
    ver_sizer->Add( 0, 0, 0, wxTOP, FromDIP(43));

    ver_sizer->Add(info_sizer, 0, wxALL | wxEXPAND, FromDIP(0));

    wxBoxSizer *copyright_ver_sizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *copyright_hor_sizer = new wxBoxSizer(wxHORIZONTAL);

    copyright_hor_sizer->Add(copyright_ver_sizer, 0, wxLEFT, FromDIP(15));

    wxStaticText *software_copyright_text = new wxStaticText(this, wxID_ANY, _L("sofrware_copyright"), wxDefaultPosition, wxDefaultSize);
    software_copyright_text->SetBackgroundColour(font_bg);

    copyright_ver_sizer->Add(software_copyright_text, 0, wxALL , 0);

    //Add "Portions copyright" button
    Button* button_portions = new Button(this,_L("Portions copyright"));
    StateColor report_bg(std::pair<wxColour, int>(wxColour(255, 255, 255), StateColor::Disabled), std::pair<wxColour, int>(wxColour(206, 206, 206), StateColor::Pressed),
                         std::pair<wxColour, int>(wxColour(238, 238, 238), StateColor::Hovered), std::pair<wxColour, int>(wxColour(255, 255, 255), StateColor::Enabled),
                         std::pair<wxColour, int>(wxColour(255, 255, 255), StateColor::Normal));
    button_portions->SetBackgroundColor(report_bg);
    StateColor report_bd(std::pair<wxColour, int>(wxColour(144, 144, 144), StateColor::Disabled), std::pair<wxColour, int>(wxColour(38, 46, 48), StateColor::Enabled));
    button_portions->SetBorderColor(report_bd);
    StateColor report_text(std::pair<wxColour, int>(wxColour(144, 144, 144), StateColor::Disabled), std::pair<wxColour, int>(wxColour(38, 46, 48), StateColor::Enabled));
    button_portions->SetTextColor(report_text);
    button_portions->SetFont(Label::Body_12);
    button_portions->SetCornerRadius(FromDIP(12));
    button_portions->SetMinSize(wxSize(FromDIP(120), FromDIP(24)));

    wxBoxSizer *copyright_button_ver = new wxBoxSizer(wxVERTICAL);
    copyright_button_ver->Add( 0, 0, 0, wxTOP, FromDIP(10));
    copyright_button_ver->Add(button_portions, 0, wxALL,0);

    copyright_hor_sizer->AddStretchSpacer();
    copyright_hor_sizer->Add(copyright_button_ver, 0, wxRIGHT, FromDIP(20));

    ver_sizer->Add(copyright_hor_sizer, 0, wxEXPAND ,0);
    ver_sizer->Add( 0, 0, 0, wxTOP, FromDIP(30));
    button_portions->Bind(wxEVT_BUTTON, &AboutDialog::onCopyrightBtn, this);

    wxGetApp().UpdateDlgDarkUI(this);
	SetSizer(main_sizer);
    Layout();
    Fit();
    CenterOnParent();
}

void AboutDialog::on_dpi_changed(const wxRect &suggested_rect)
{
    m_logo_bitmap.msw_rescale();
    m_logo->SetBitmap(m_logo_bitmap.bmp());

    const wxFont& font = GetFont();
    const int fs = font.GetPointSize() - 1;
    int font_size[] = { fs, fs, fs, fs, fs, fs, fs };
    m_website_html->SetFonts(font.GetFaceName(), font.GetFaceName(), font_size);
    m_email_html->SetFonts(font.GetFaceName(), font.GetFaceName(), font_size);

    const int& em = em_unit();

    msw_buttons_rescale(this, em, { wxID_CLOSE, m_copy_rights_btn_id });

    m_website_html->SetMinSize(wxSize(-1, 16 * em));
    m_website_html->Refresh();

    m_email_html->SetMinSize(wxSize(-1, 16 * em));
    m_email_html->Refresh();

    const wxSize& size = wxSize(65 * em, 30 * em);

    SetMinSize(size);
    Fit();
    Refresh();
}

void AboutDialog::onLinkClicked(wxHtmlLinkEvent &event)
{
    wxGetApp().open_browser_with_warning_dialog(event.GetLinkInfo().GetHref());
    event.Skip(false);
}

void AboutDialog::onCloseDialog(wxEvent &)
{
    this->EndModal(wxID_CLOSE);
}

void AboutDialog::onCopyrightBtn(wxEvent &)
{
    CopyrightsDialog dlg;
    dlg.ShowModal();
}

void AboutDialog::onCopyToClipboard(wxEvent&)
{
    wxTheClipboard->Open();
    wxTheClipboard->SetData(new wxTextDataObject(_L("Version") + " " + GUI_App::format_display_version()));
    wxTheClipboard->Close();
}

} // namespace GUI
} // namespace Slic3r
