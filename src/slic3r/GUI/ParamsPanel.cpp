///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.0-4761b0c)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////
#include "Widgets/StateColor.hpp"
#include "libslic3r/PresetBundle.hpp"
#include "libslic3r/Preset.hpp"

#include "Tab.hpp"
#include "format.hpp"
#include "MainFrame.hpp"
#include "GUI_App.hpp"
#include "Plater.hpp"

#include "libslic3r/AppConfig.hpp"
#include "Widgets/Label.hpp"
#include "Widgets/SwitchButton.hpp"
#include "Widgets/Button.hpp"
#include "Widgets/HoverBorderBox.hpp"
#include "Widgets/HoverBorderIcon.hpp"
#include "GUI_Factories.hpp"
#include "Widgets/TabCtrl.hpp"
#include "ParamsPanel.hpp"
#include <map>
#include <wx/colour.h>
#include <wx/event.h>
#include <wx/gdicmn.h>
#include <wx/brush.h>
#include <wx/colour.h>
#include <wx/string.h>
#include <wx/wx.h>
#include <wx/dataview.h>


namespace Slic3r {
namespace GUI {

    ImageTooltipPanel::ImageTooltipPanel(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style)
        : wxPanel(parent, id, pos, size, style), m_backgroundImage(nullptr)
    {
        // Bind the paint event to the OnPaint method
        Bind(wxEVT_PAINT, &ImageTooltipPanel::OnPaint, this);

        static Slic3r::GUI::BitmapCache cache;

        // Initialize layout
        m_mainSizer = new wxBoxSizer(wxVERTICAL);
        this->SetSizer(m_mainSizer);

        m_textButtonSizer = new wxBoxSizer(wxHORIZONTAL);

        // const wxColour grey_color(54, 54, 56);
        bool is_dark = Slic3r::GUI::wxGetApp().dark_mode();
        // wxColour bg_color = is_dark ? wxColour("#363638") : wxColour("#FFFFFF");
        wxColour bg_color = is_dark ? wxColour(75,75,77) : wxColour("#FFFFFF");

        // Initialize the static text
        m_text = new wxStaticText(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);
        m_text->SetBackgroundColour(bg_color);

        m_textButtonSizer->AddStretchSpacer(1); // Add stretchable space before m_text
        m_textButtonSizer->Add(m_text, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);

        m_textButtonSizer->AddStretchSpacer(1); // Add stretchable space before m_hideButton

        m_hideButton = new ScalableButton(this, wxID_ANY, is_dark ? "button_hide_img_tooltip" : "button_hide_img_light", wxEmptyString, wxDefaultSize, 
                                  wxDefaultPosition, wxBU_EXACTFIT | wxNO_BORDER, true, 10, true);

        m_hideButton->SetBackgroundColour(wxColour(54, 154, 56)); // Set button background color to grey
        m_textButtonSizer->Add(m_hideButton, 0, wxALIGN_RIGHT | wxALL, 5);

        m_hideButton->Bind(wxEVT_BUTTON, &ImageTooltipPanel::OnHideButtonClick, this);

        this->GetSizer()->Add(m_textButtonSizer, 0, wxEXPAND | wxALL, 5);

        SetBackgroundColour(bg_color);
        SetBackgroundStyle(wxBG_STYLE_PAINT); // Enable custom background painting

        m_defaultBmp = cache.load_png("image_tooltip_default");
        if(m_defaultBmp) {
            m_backgroundImage = new wxStaticBitmap(this, wxID_ANY, *m_defaultBmp);
            m_backgroundImage->SetBackgroundColour(is_dark ? wxColour(54,54,56) : wxColour("#FFFFFF"));
            this->GetSizer()->Add(m_backgroundImage, 1, wxEXPAND | wxALL, 5);
        }

    }

    ImageTooltipPanel::~ImageTooltipPanel()
    {
        if (m_backgroundImage) {
            m_backgroundImage->Destroy();
        }

        if (m_text) {
            m_text->Destroy();
        }

        if (m_textButtonSizer) {  
            m_textButtonSizer->Clear();
        }

        if(m_mainSizer) {
            m_mainSizer->Clear();
        }
    }

    void ImageTooltipPanel::OnHideButtonClick(wxCommandEvent& event)
    {
        ParamsPanel* paramsPanel = dynamic_cast<ParamsPanel*>(this->GetParent());
        if(paramsPanel) {
            paramsPanel->tab_page_relayout();
        }
    }

    void ImageTooltipPanel::OnPaint(wxPaintEvent& event)
    {
        wxAutoBufferedPaintDC dc(this);
        dc.Clear();

        wxRect rect = GetClientRect();

        bool is_dark = wxGetApp().dark_mode();
        wxColour fill_bg_color = is_dark ? wxColour(54,54,56) : wxColour("#FFFFFF");

        wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
        if (gc) {
            gc->SetPen(wxPen(is_dark ? fill_bg_color : wxColour(214, 214, 220), 1));
            // gc->SetBrush(*wxTRANSPARENT_BRUSH);
            gc->SetBrush(fill_bg_color);

            // Create a rounded rectangle path with all four corners rounded
            wxGraphicsPath path = gc->CreatePath();
            path.AddRoundedRectangle(rect.x, rect.y, rect.width , rect.height , 12);

            gc->DrawPath(path);
            delete gc;
        }

        event.Skip();
    }

    void ImageTooltipPanel::SetBackgroundImage(const wxString& imagePath, const wxString& textTooltip)
    {
        static Slic3r::GUI::BitmapCache cache;

        unsigned int width  = 0;
        unsigned int height = 0;

        if(!imagePath.empty()) {
            if(m_imagePath == imagePath) {
                return;
            }
            m_imagePath = imagePath;
        }
        else {
            m_imagePath = "";
        }

        wxBitmap* bmp = nullptr;

        // Check if the imagePath ends with ".svg"
        if (imagePath.EndsWith(".svg")) {
            wxString basePath = imagePath.BeforeLast('.');
            bmp               = cache.load_svg(basePath.ToStdString(), width, height);
        } else {
            // If not SVG, try loading as PNG
            wxImage image;
            if (image.LoadFile(imagePath, wxBITMAP_TYPE_PNG)) {
                bmp = new wxBitmap(image);
            }
        }

        if (bmp) {
            if (!m_backgroundImage) {
                m_backgroundImage = new wxStaticBitmap(this, wxID_ANY, *bmp,wxDefaultPosition, wxSize(50,50));
                this->GetSizer()->Add(m_backgroundImage, 1, wxEXPAND | wxALL, 0);
            } else {
                m_backgroundImage->SetBitmap(*bmp);
            }
        } else {
            m_backgroundImage->SetBitmap(*m_defaultBmp);
        }

        if (!m_text) {
            m_text = new wxStaticText(this, wxID_ANY, textTooltip);
            this->GetSizer()->Add(m_text, 0, wxALL | wxEXPAND, 5);
        } else {
            m_text->SetLabel(textTooltip);
        }

        this->Layout();
        this->Show();
    }

    void ImageTooltipPanel::on_change_color_mode(bool is_dark)
    {
        wxColour text_btn_bg_color = is_dark ? wxColour(54,54,56) : wxColour("#FFFFFF");

        if(m_hideButton) {
            wxBitmap button_bitmap = create_scaled_bitmap(is_dark ? "button_hide_img_tooltip" : "button_hide_img_light", this, 10);
            m_hideButton->SetBitmap(button_bitmap);
            m_hideButton->SetBackgroundColour(text_btn_bg_color); 
        }

        if(m_text)
            m_text->SetBackgroundColour(text_btn_bg_color);

        if(m_backgroundImage)
            m_backgroundImage->SetBackgroundColour(is_dark ? wxColour(54,54,56) : wxColour("#FFFFFF"));

        wxColour bg_color = is_dark ? wxColour(75,75,77) : wxColour("#FFFFFF");
        SetBackgroundColour(bg_color);
    }

TipsDialog::TipsDialog(wxWindow *parent, const wxString &title, const wxString &description, std::string app_key)
    : DPIDialog(parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize, wxCAPTION | wxCLOSE_BOX),
    m_app_key(app_key)
{
    SetBackgroundColour(*wxWHITE);
    std::string icon_path = (boost::format("%1%/images/Creative3DTitle.ico") % resources_dir()).str();
    SetIcon(wxIcon(encode_path(icon_path.c_str()), wxBITMAP_TYPE_ICO));

    wxBoxSizer *m_sizer_main = new wxBoxSizer(wxVERTICAL);

    m_top_line = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
    m_top_line->SetBackgroundColour(wxColour(166, 169, 170));

    m_sizer_main->Add(m_top_line, 0, wxEXPAND, 0);

    m_sizer_main->Add(0, 0, 0, wxEXPAND | wxTOP, FromDIP(20));

    m_msg = new wxStaticText(this, wxID_ANY, description, wxDefaultPosition, wxDefaultSize, 0);
    m_msg->Wrap(-1);
    m_msg->SetFont(::Label::Body_13);
    m_msg->SetForegroundColour(wxColour(107, 107, 107));
    m_msg->SetBackgroundColour(wxColour(255, 255, 255));

    m_sizer_main->Add(m_msg, 1, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(40));

    m_sizer_main->Add(0, 0, 0, wxEXPAND | wxTOP, FromDIP(5));

    wxBoxSizer *m_sizer_bottom = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer *m_sizer_left   = new wxBoxSizer(wxHORIZONTAL);

    auto dont_show_again = create_item_checkbox(_L("Don't show again"), this, _L("Don't show again"), "do_not_show_tips");
    m_sizer_left->Add(dont_show_again, 1, wxALL, FromDIP(5));

    m_sizer_bottom->Add(m_sizer_left, 1, wxEXPAND, FromDIP(5));

    wxBoxSizer *m_sizer_right = new wxBoxSizer(wxHORIZONTAL);

    m_confirm = new Button(this, _L("OK"));
    StateColor btn_bg_grey(
        std::pair<wxColour, int>(wxColour(206, 206, 206), StateColor::Pressed),
        std::pair<wxColour, int>(wxColour(150, 150, 155), StateColor::Hovered),
        std::pair<wxColour, int>(wxColour(255, 255, 255), StateColor::Normal)
    );

    StateColor btn_text_white(
        std::pair<wxColour, int>(wxColour(38, 46, 48), StateColor::Normal)
    );

    StateColor btn_bd_grey(
        std::pair<wxColour, int>(wxColour(38, 46, 48), StateColor::Normal)
    );

    m_confirm->SetBackgroundColor(btn_bg_grey);
    m_confirm->SetBorderColor(btn_bd_grey);
    m_confirm->SetTextColor(btn_text_white);
    m_confirm->SetFocus();
    m_confirm->SetSize(TIPS_DIALOG_BUTTON_SIZE);
    m_confirm->SetMinSize(TIPS_DIALOG_BUTTON_SIZE);
    m_confirm->SetCornerRadius(FromDIP(12));
    m_confirm->Bind(wxEVT_LEFT_DOWN, &TipsDialog::on_ok, this);
    m_sizer_right->Add(m_confirm, 0, wxALL, FromDIP(5));

    m_sizer_bottom->Add(m_sizer_right, 0, wxEXPAND, FromDIP(5));
    m_sizer_main->Add(m_sizer_bottom, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(40));
    m_sizer_main->Add(0, 0, 0, wxEXPAND | wxTOP, FromDIP(20));

    SetSizer(m_sizer_main);
    Layout();
    Fit();
    Centre(wxBOTH);

    wxGetApp().UpdateDlgDarkUI(this);
}

wxBoxSizer *TipsDialog::create_item_checkbox(wxString title, wxWindow *parent, wxString tooltip, std::string param)
{
    wxBoxSizer *m_sizer_checkbox = new wxBoxSizer(wxHORIZONTAL);

    m_sizer_checkbox->Add(0, 0, 0, wxEXPAND | wxLEFT, 5);

    auto checkbox = new ::CheckBox(parent);
    m_sizer_checkbox->Add(checkbox, 0, wxALIGN_CENTER, 0);
    m_sizer_checkbox->Add(0, 0, 0, wxEXPAND | wxLEFT, 8);

    auto checkbox_title = new wxStaticText(parent, wxID_ANY, title, wxDefaultPosition, wxSize(-1, -1), 0);
    checkbox_title->SetForegroundColour(wxColour(144, 144, 144));
    checkbox_title->SetFont(::Label::Body_13);
    checkbox_title->Wrap(-1);
    m_sizer_checkbox->Add(checkbox_title, 0, wxALIGN_CENTER | wxALL, 3);

    m_show_again = wxGetApp().app_config->get(param) == "true" ? true : false;
    checkbox->SetValue(m_show_again);

    checkbox->Bind(wxEVT_TOGGLEBUTTON, [this, checkbox, param](wxCommandEvent &e) {
        m_show_again = m_show_again ? false : true;
        e.Skip();
    });

    return m_sizer_checkbox;
}

void TipsDialog::on_dpi_changed(const wxRect &suggested_rect)
{
    if (m_confirm) m_confirm->SetMinSize(TIPS_DIALOG_BUTTON_SIZE);
    if (m_cancel) m_cancel->SetMinSize(TIPS_DIALOG_BUTTON_SIZE);
    Fit();
    Refresh();
}

void TipsDialog::on_ok(wxMouseEvent &event)
{
    if (m_show_again) {
        if (!m_app_key.empty())
        wxGetApp().app_config->set_bool(m_app_key, m_show_again);
    }
    EndModal(wxID_OK);
}

void ParamsPanel::Highlighter::set_timer_owner(wxEvtHandler *owner, int timerid /* = wxID_ANY*/)
{
    m_timer.SetOwner(owner, timerid);
}

void ParamsPanel::Highlighter::init(std::pair<wxWindow *, bool *> params, wxWindow *parent)
    {
    if (m_timer.IsRunning()) invalidate();
    if (!params.first || !params.second) return;

    m_timer.Start(300, false);

    m_bitmap         = params.first;
    m_show_blink_ptr = params.second;
    m_parent         = parent;

    *m_show_blink_ptr = true;
    }

void ParamsPanel::Highlighter::invalidate()
{
    m_timer.Stop();

    if (m_bitmap && m_show_blink_ptr) {
        *m_show_blink_ptr = false;
        m_bitmap->Show(*m_show_blink_ptr);
        if (m_parent) {
            m_parent->Layout();
            m_parent->Refresh();
        }
        m_show_blink_ptr = nullptr;
        m_bitmap         = nullptr;
        m_parent         = nullptr;
    }

    m_blink_counter = 0;
}

void ParamsPanel::Highlighter::blink()
{
    if (m_bitmap && m_show_blink_ptr) {
        *m_show_blink_ptr = !*m_show_blink_ptr;
        m_bitmap->Show(*m_show_blink_ptr);
        if (m_parent) {
            m_parent->Layout();
            m_parent->Refresh();
        }
    } else
        return;

    if ((++m_blink_counter) == 11) invalidate();
}

ParamsPanel::ParamsPanel( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name )
    : wxPanel( parent, id, pos, size, style, name )
{
    // BBS: new layout
    SetBackgroundColour(*wxWHITE);
//#if __WXOSX__
//    m_top_sizer = new wxBoxSizer(wxHORIZONTAL);
//    m_top_sizer->SetSizeHints(this);
//    this->SetSizer(m_top_sizer);
//
//    // Create additional panel to Fit() it from OnActivate()
//    // It's needed for tooltip showing on OSX
//    m_tmp_panel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBK_LEFT | wxTAB_TRAVERSAL);
//    auto  sizer = new wxBoxSizer(wxHORIZONTAL);
//    m_tmp_panel->SetSizer(sizer);
//    m_tmp_panel->Layout();
//
//#else
    ParamsPanel*panel = this;
    m_top_sizer = new wxBoxSizer(wxVERTICAL);
    m_top_sizer->SetSizeHints(panel);
    panel->SetSizer(m_top_sizer);
//#endif //__WXOSX__
     bool is_dark = wxGetApp().dark_mode();
    if (dynamic_cast<Notebook*>(parent)) {
        // BBS: new layout
        m_top_panel = new StaticBox(this, wxID_ANY, wxDefaultPosition);
        m_top_panel->SetBackgroundColor(0xFFFFFF);
        m_top_panel->SetBackgroundColor2(0xFFFFFF);

        m_process_icon = new ScalableButton(m_top_panel, wxID_ANY, is_dark?"process_dark_default":"process_light_default");

        m_title_label = new Label(m_top_panel, Label::Head_14, _L("Process"));

        //int width, height;
        // BBS: new layout
        m_mode_region = new SwitchButton(m_top_panel);
        m_mode_region->SetMaxSize({em_unit(this) * 12, -1});
        m_mode_region->SetLabels(_L("Global"), _L("Objects"));
        //m_mode_region->GetSize(&width, &height);
        m_tips_arrow = new ScalableButton(m_top_panel, wxID_ANY, "tips_arrow");
        m_tips_arrow->Hide();

        m_mode_view  = new SwitchButton(m_top_panel, wxID_ABOUT, is_dark ? "advanced_process_dark_checked" : "advanced_process_light_checked",
                                       is_dark ? "advanced_process_dark" : "advanced_process_light", true, wxSize(FromDIP(10), FromDIP(10)));
        
        m_mode_view_box = new HoverBorderBox(m_top_panel, m_mode_view, wxDefaultPosition, wxSize(FromDIP(24),FromDIP(24)), wxTE_PROCESS_ENTER);
        m_mode_view->SetToolTip(_L("Advance parameters"));

        //reset
        m_btn_reset = new ScalableButton(m_top_panel, wxID_ANY, is_dark ? "undo" : "undo", wxEmptyString,
                                           wxDefaultSize, wxDefaultPosition, wxBU_EXACTFIT | wxNO_BORDER, true, FromDIP(14));
        m_btn_reset->SetToolTip(_L("Reset"));
        m_btn_reset->Bind(wxEVT_BUTTON, ([this](wxCommandEvent e) {
                              for (auto tab : {m_tab_print, m_tab_print_plate, m_tab_print_object, m_tab_print_part, m_tab_print_layer,
                                               m_tab_filament, m_tab_printer}) {
                                  if (tab)
                                  {
                                      Tab* pTab = dynamic_cast<Tab*>(tab);
                                      if (nullptr != pTab)
                                          pTab->on_roll_back_value();

                                      TabPrintModel* pTabM = dynamic_cast<TabPrintModel*>(tab);
                                      if (nullptr != pTabM)
                                          pTabM->reset_model_config();

                                      wxGetApp().params_panel()->notify_object_config_changed();
                                  }
                              }

            }));

        m_btn_reset->Hide();  // no need to show it
         

        // BBS: new layout
        //m_search_btn = new ScalableButton(m_top_panel, wxID_ANY, "search", wxEmptyString, wxDefaultSize, wxDefaultPosition, wxBU_EXACTFIT | wxNO_BORDER, true);
        //m_search_btn->SetToolTip(format_wxstr(_L("Search in settings [%1%]"), "Ctrl+F"));
        //m_search_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent &) { wxGetApp().plater()->search(false); });

        m_compare_btn =  new  HoverBorderIcon(m_top_panel, wxEmptyString, is_dark ? "compare_dark_default" : "compare_light_default", wxDefaultPosition, wxSize(FromDIP(24), FromDIP(24)), wxTE_PROCESS_ENTER);
        m_compare_btn->SetToolTip(_L("Compare presets"));
        m_compare_btn->Bind(wxEVT_LEFT_DOWN, ([this](auto& e) { wxGetApp().mainframe->diff_dialog.show(); }));

        // m_setting_btn = new HoverBorderIcon(m_top_panel, wxID_ANY, "table", wxEmptyString, wxDefaultSize, wxDefaultPosition, wxBU_EXACTFIT | wxNO_BORDER, true, 16, true, wxSize(10, 10));
        m_setting_btn = new  HoverBorderIcon(m_top_panel, wxEmptyString, "table", wxDefaultPosition, wxSize(FromDIP(24), FromDIP(24)), wxTE_PROCESS_ENTER);
        m_setting_btn->SetToolTip(_L("View all object's settings"));
        m_setting_btn->Bind(wxEVT_LEFT_DOWN, [this](auto& e) { wxGetApp().plater()->PopupObjectTable(-1, -1, {0, 0}); });

        m_highlighter.set_timer_owner(this, 0);
        this->Bind(wxEVT_TIMER, [this](wxTimerEvent &)
        {
            m_highlighter.blink();
        });
    }



    //m_export_to_file = new Button( this, wxT("Export To File"), "");
    //m_import_from_file = new Button( this, wxT("Import From File") );

    // Initialize the page.
//#if __WXOSX__
//    auto page_parent = m_tmp_panel;
//#else
    auto page_parent = this;
//#endif

    m_color_border_box = new StaticBox(this, wxID_ANY, wxDefaultPosition);
    m_color_border_box->SetBackgroundColour(*wxWHITE);
    m_color_border_box->SetBorderWidth(1);
    m_color_border_box->SetBorderColor(0x7A7A7F);
    m_tmp_sizer = new wxBoxSizer(wxVERTICAL);
    m_tmp_sizer->SetMinSize(wxSize(0, FromDIP(150)));

    // BBS: fix scroll to tip view
    class PageScrolledWindow : public wxScrolledWindow
    {
    public:
        PageScrolledWindow(wxWindow* parent)
            : wxScrolledWindow(parent,
                wxID_ANY,
                wxDefaultPosition,
                wxDefaultSize,
                wxVSCROLL) // hide hori-bar will cause hidden field mis-position
        {
            // ShowScrollBar(GetHandle(), SB_BOTH, FALSE);
            Bind(wxEVT_SCROLL_CHANGED, [this](auto& e) {
                wxWindow* child = dynamic_cast<wxWindow*>(e.GetEventObject());
                if (child != this)
                    EnsureVisible(child);
                });
        }
        virtual bool ShouldScrollToChildOnFocus(wxWindow* child)
        {
            EnsureVisible(child);
            return false;
        }
        void EnsureVisible(wxWindow* win)
        {
            const wxRect viewRect(m_targetWindow->GetClientRect());
            const wxRect winRect(m_targetWindow->ScreenToClient(win->GetScreenPosition()), win->GetSize());
            if (viewRect.Contains(winRect)) {
                return;
            }
            if (winRect.GetWidth() > viewRect.GetWidth() || winRect.GetHeight() > viewRect.GetHeight()) {
                return;
            }
            int stepx, stepy;
            GetScrollPixelsPerUnit(&stepx, &stepy);

            int startx, starty;
            GetViewStart(&startx, &starty);
            // first in vertical direction:
            if (stepy > 0) {
                int diff = 0;

                if (winRect.GetTop() < 0) {
                    diff = winRect.GetTop();
                }
                else if (winRect.GetBottom() > viewRect.GetHeight()) {
                    diff = winRect.GetBottom() - viewRect.GetHeight() + 1;
                    // round up to next scroll step if we can't get exact position,
                    // so that the window is fully visible:
                    diff += stepy - 1;
                }
                starty = (starty * stepy + diff) / stepy;
            }
            // then horizontal:
            if (stepx > 0) {
                int diff = 0;
                if (winRect.GetLeft() < 0) {
                    diff = winRect.GetLeft();
                }
                else if (winRect.GetRight() > viewRect.GetWidth()) {
                    diff = winRect.GetRight() - viewRect.GetWidth() + 1;
                    // round up to next scroll step if we can't get exact position,
                    // so that the window is fully visible:
                    diff += stepx - 1;
                }
                startx = (startx * stepx + diff) / stepx;
            }
            Scroll(startx, starty);
        }
    };

    // m_page_view = new PageScrolledWindow(page_parent);
    m_page_view = new PageScrolledWindow(m_color_border_box);
    m_page_view->SetBackgroundColour(*wxWHITE);
    // m_page_view->SetWindowStyle(wxBORDER_SIMPLE);
    m_page_sizer = new wxBoxSizer(wxVERTICAL);

    m_page_view->SetSizer(m_page_sizer);
    m_page_view->SetScrollbars(1, 20, 1, 2);
    //m_page_view->SetScrollRate( 5, 5 );

    m_tmp_sizer->Add(m_page_view, 1, wxEXPAND | wxALL, FromDIP(10));

    //layoutPrinterAndFilament();
    //layoutOther();

    //const std::deque<Preset>& presets = wxGetApp().preset_bundle->printers.get_presets();
    //Preset preset = presets[0];

    //std::string inheritsPrint = preset.config.opt_string("inherits");
    //std::string ventor = getVendor(preset);

    //m_curVentor = ventor;
    //m_printerType = inheritsPrint;

}

void ParamsPanel::create_layout()
{
    layoutPrinterAndFilament();
    create_layout_printerAndFilament();
    //if (m_current_tab)
    //{
    //    Preset::Type type = dynamic_cast<Tab*>(m_current_tab)->type();
    //    if (type == Preset::TYPE_PRINTER || type == Preset::TYPE_PRINTER)
    //    {
    //        create_layout_printerAndFilament();
    //    }
    //    else
    //    {
    //        create_layout_process();
    //    }
    //}
    //else 
    //{
    //    create_layout_process();
    //}
    
}

void ParamsPanel::create_layout_printerAndFilament()
{
//#ifdef __WINDOWS__
//    this->SetDoubleBuffered(true);
//    m_page_view->SetDoubleBuffered(true);
//#endif //__WINDOWS__

    m_left_sizer = new wxBoxSizer(wxHORIZONTAL);
    // BBS: new layout
    m_left_sizer->SetMinSize(wxSize(40 * em_unit(this), -1));

    if (m_top_panel) {
        m_mode_sizer = new wxBoxSizer(wxHORIZONTAL);
        m_mode_sizer->AddSpacer(FromDIP(11));
        m_mode_sizer->Add(m_process_icon, 0, wxALIGN_CENTER);
        m_mode_sizer->AddSpacer(FromDIP(11));
        m_mode_sizer->Add(m_title_label, 0, wxALIGN_CENTER);
        m_mode_sizer->AddStretchSpacer(2);
        m_mode_sizer->Add(m_mode_region, 0, wxALIGN_CENTER);
        m_mode_sizer->AddStretchSpacer(1);
        m_mode_sizer->Add(m_tips_arrow, 0, wxALIGN_CENTER);
        m_mode_sizer->AddStretchSpacer(10);
        //         m_mode_sizer->Add( m_title_view, 0, wxALIGN_CENTER );
        //         m_mode_sizer->AddSpacer(FromDIP(2));

        m_mode_sizer->AddStretchSpacer(10);
        m_mode_sizer->Add(m_mode_view_box, 0, wxALIGN_CENTER);
        m_mode_sizer->AddSpacer(FromDIP(14));
        // m_mode_sizer->Add(m_btn_reset, 0, wxALIGN_CENTER);  // the m_btn_reset is not used
        // m_mode_sizer->AddSpacer(FromDIP(14));
        m_mode_sizer->Add(m_setting_btn, 0, wxALIGN_CENTER);
        m_mode_sizer->AddSpacer(FromDIP(14));
        m_mode_sizer->Add(m_compare_btn, 0, wxALIGN_CENTER);

        m_mode_sizer->AddSpacer(FromDIP(15));
        //m_mode_sizer->Add( m_search_btn, 0, wxALIGN_CENTER );
        //m_mode_sizer->AddSpacer(16);
        m_mode_sizer->SetMinSize(-1, FromDIP(35));
        m_top_panel->SetSizer(m_mode_sizer);
        //m_left_sizer->Add( m_top_panel, 0, wxEXPAND );
    }

    if (m_tab_print) {
        m_left_sizer->Add(m_tab_print, 0, wxEXPAND);
    }

    if (m_tab_print_plate) {
        m_left_sizer->Add(m_tab_print_plate, 0, wxEXPAND);
    }

    if (m_tab_print_object) {
        m_left_sizer->Add(m_tab_print_object, 0, wxEXPAND);
    }

    if (m_tab_print_part) {
        m_left_sizer->Add(m_tab_print_part, 0, wxEXPAND);
    }

    if (m_tab_print_layer) {
        m_left_sizer->Add(m_tab_print_layer, 0, wxEXPAND);	
    }

    {
        const std::deque<Preset>& presets = wxGetApp().preset_bundle->printers.get_presets();
        size_t count = wxGetApp().preset_bundle->printers.num_default_presets();
        const Preset* pt = wxGetApp().preset_bundle->printers.find_system_preset_by_model_and_variant("1", "3");

        //m_preset_listBox = new wxTreeCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, nullptr, wxBORDER_NONE);
        m_preset_listBox = new wxDataViewTreeCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_NO_HEADER | wxBORDER_NONE);

        //// 创建列
        //m_preset_listBox->AppendTextColumn("Name", 0, wxDATAVIEW_CELL_INERT, 200);
        //wxDataViewColumn* col = new wxDataViewColumn("Value", new MyRenderer("string"), 1, 200);
        //m_preset_listBox->AppendColumn(col);

        m_preset_listBox->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, [this](wxDataViewEvent& event) {
            wxDataViewItem item = event.GetItem();
            if (item.IsOk()) {
                std::string itemString = m_preset_listBox->GetItemText(item).ToUTF8().data();;
                m_curPreset = m_preset_listBox->GetItemText(item);
                if (itemString == _L("User presets") || itemString == _L("System presets") || itemString == _L("Project presets"))
                {
                    return;
                }
                //dynamic_cast<TabPrinter*>(m_tab_printer)->changedSelectPrint(itemString);
                if (m_ws == WS_PRINTER)
                    dynamic_cast<TabPrinter*>(m_tab_printer)->changedSelectPrint(itemString);
                else
                    dynamic_cast<TabFilament*>(m_tab_filament)->changedSelectFilament(itemString);

            }
            updateItemState();
            });

        size_t idx_selected = wxGetApp().preset_bundle->printers.get_selected_idx();
        const std::map<std::string, ProfileMachine>& profile_machines = wxGetApp().app_config->get_profile_machines();
        size_t size = profile_machines.size();
        AppConfig* app_config = wxGetApp().app_config;
        app_config->get_userPresets("CR-10 SE");

        m_preset_listBox->SetMinSize(wxSize(FromDIP(260), -1));
        m_left_sizer->Add(m_preset_listBox, 1, wxEXPAND | wxLEFT, FromDIP(20));

        //wxString title = cur_tab->type() == Preset::TYPE_FILAMENT ? _L("Filament settings") : _L("Printer settings");
        //wxPanel* tabCtrPanel = new wxPanel(this);
        StaticBox* tabCtrPanel = new StaticBox(this, wxID_ANY, wxDefaultPosition, wxDefaultSize);
        tabCtrPanel->SetBorderWidth(1);
        tabCtrPanel->SetBorderColor(0x7A7A7F);
        tabCtrPanel->SetMinSize(wxSize(116, -1));
        wxBoxSizer* tabCtr_sizer = new wxBoxSizer(wxVERTICAL);
        tabCtrPanel->SetSizer(tabCtr_sizer);
        if (m_tab_printer) {
            m_printer_tabCtrl = dynamic_cast<Tab*>(m_tab_printer)->get_tabCtrl();
            //m_printer_tabCtrl->SetBackgroundColour(0xffffff);
            //m_printer_tabCtrl->SetMinSize(wxSize(116, -1));
            m_printer_tabCtrl->Reparent(tabCtrPanel);
            //m_left_sizer->Add(m_printer_tabCtrl, 0, wxEXPAND | wxALL, 5);
            tabCtr_sizer->Add(m_printer_tabCtrl, 0, wxEXPAND | wxALL, 5);
        }
        if (m_tab_filament) {
            m_filament_tabCtrl = dynamic_cast<Tab*>(m_tab_filament)->get_tabCtrl();
            //m_filament_tabCtrl->SetBackgroundColour(0xffffff);
            //m_filament_tabCtrl->SetMinSize(wxSize(116, -1));
            m_filament_tabCtrl->Reparent(tabCtrPanel);
            //m_left_sizer->Add(m_filament_tabCtrl, 0, wxEXPAND | wxALL, 5);
            tabCtr_sizer->Add(m_filament_tabCtrl, 0, wxEXPAND | wxALL, 5);
        }

        tabCtr_sizer->AddStretchSpacer();
        m_left_sizer->Add(tabCtrPanel, 1, wxEXPAND | wxLEFT | wxRIGHT, 5);

        m_btnsPanel = new wxPanel(this);
        wxBoxSizer* buttons_sizer = new wxBoxSizer(wxHORIZONTAL);
        m_btnsPanel->SetSizer(buttons_sizer);

        bool is_dark = wxGetApp().dark_mode();

        int em = em_unit(this);
        StateColor text_color = StateColor(std::pair{ is_dark ? wxColour(254, 254, 254) : wxColour(0,0,0), (int)StateColor::Normal });
        StateColor bg_color = StateColor(std::pair{ wxColour(21, 191, 89), (int)StateColor::Hovered },
            std::pair{ is_dark ? wxColour(1, 1, 1) : wxColour(214, 214, 220), (int)StateColor::Normal });


        wxFont font(12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("微软雅黑"));

        m_btn_system = new wxButton(m_btnsPanel, wxID_ANY, _L("System"), wxDefaultPosition, wxDefaultSize,
            wxLeft | wxBORDER_NONE);
        m_btn_user = new wxButton(m_btnsPanel, wxID_ANY, _L("User"), wxDefaultPosition, wxDefaultSize,
            wxLeft | wxBORDER_NONE);

        buttons_sizer->Add(m_btn_system);
        m_btn_system->SetMinSize({ FromDIP(100), FromDIP(30)});
        m_btn_system->SetBackgroundColour(wxColour(21, 191, 89));
        m_btn_system->SetClientData(new bool(true));
        m_btn_system->SetFont(font);
        m_btn_system->Bind(wxEVT_ENTER_WINDOW, [this](auto& event) {
            bool* sys_Value = static_cast<bool*>(m_btn_system->GetClientData());
            if (*sys_Value)
                return;
            m_btn_system->SetBackgroundColour(wxColour(21, 191, 89));
            });
        m_btn_system->Bind(wxEVT_LEAVE_WINDOW, [this](auto& event) {
            bool* sys_Value = static_cast<bool*>(m_btn_system->GetClientData());
            if (*sys_Value)
                return;
            m_btn_system->SetBackgroundColour(wxColour(214, 214, 220));
            });

        m_btn_system->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event) {

            m_ps = ParamsPanel::PS_SYSTEM;
            m_curVentor = _L("ALL").ToStdString();
            m_printerType = _L("ALL").ToStdString();
            filteredData(m_curVentor, m_printerType);
            updateItemState();
            });
        Slic3r::GUI::wxGetApp().UpdateDarkUI(m_btn_system);

        buttons_sizer->Add(m_btn_user);
        m_btn_user->SetMinSize({ FromDIP(100), FromDIP(30)});
        m_btn_user->SetBackgroundColour(wxColour(214, 214, 220));
        m_btn_user->SetClientData(new bool(false));
        m_btn_user->SetFont(font);
        m_btn_user->Bind(wxEVT_ENTER_WINDOW, [this](auto& event) {
            bool* sys_Value = static_cast<bool*>(m_btn_user->GetClientData());
            if (*sys_Value)
                return;
            m_btn_user->SetBackgroundColour(wxColour(21, 191, 89));
            });
        m_btn_user->Bind(wxEVT_LEAVE_WINDOW, [this](auto& event) {
            bool* sys_Value = static_cast<bool*>(m_btn_user->GetClientData());
            if (*sys_Value)
                return;
            m_btn_user->SetBackgroundColour(wxColour(214, 214, 220));
            });
        m_btn_user->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event) {
            //bool* sys_Value = static_cast<bool*>(m_btn_system->GetClientData());
            //bool* user_Value = static_cast<bool*>(m_btn_user->GetClientData());
            //if (*user_Value)
            //{
            //    return;
            //}
            //else
            //{
            //    *user_Value = !*user_Value;
            //    *sys_Value = !*sys_Value;
            //    m_btn_user->SetBackgroundColour(wxColour(21, 191, 89));
            //    m_btn_system->SetBackgroundColour(wxColour(214, 214, 220));
            //}

            m_ps = ParamsPanel::PS_USER;
            m_curVentor = _L("ALL");
            m_printerType = _L("ALL");
            filteredData(m_curVentor, m_printerType);
            updateItemState();
            Layout();
            });

        Slic3r::GUI::wxGetApp().UpdateDarkUI(m_btn_user);

        m_btnsPanel->Layout();
        m_btnsPanel->Fit();
        m_top_sizer->Add(m_btnsPanel, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, 10);
        m_top_sizer->Add(m_tab_printer, 0, wxEXPAND);
        m_top_sizer->Add(m_tab_filament, 0, wxEXPAND);
        m_top_sizer->Add(m_left_sizer, 9, wxEXPAND | wxRight | wxLeft, 20);
        //m_top_sizer->AddStretchSpacer();

        m_bottomBtnsPanel = new wxPanel(this);
        wxBoxSizer* buttons_sizer_bt = new wxBoxSizer(wxHORIZONTAL);
        m_bottomBtnsPanel->SetSizer(buttons_sizer_bt);

        //m_btn_save = new Button(this, _L("Save"));
        m_btn_save = new wxButton(m_bottomBtnsPanel, wxID_ANY, _L("Save"), wxDefaultPosition, wxDefaultSize,
            wxLeft | wxBORDER_NONE);
        m_btn_save->SetMinSize({ FromDIP(100), FromDIP(30)});
        m_btn_save->SetBackgroundColour(m_normal_color);
        m_btn_save->SetClientData(new bool(false));
        m_btn_save->SetFont(font);
        m_btn_save->Bind(wxEVT_ENTER_WINDOW, [this](auto& event) {
            bool* sys_Value = static_cast<bool*>(m_btn_save->GetClientData());
            if (*sys_Value)
                return;
            m_btn_save->SetBackgroundColour(m_hover_color);
            });
        m_btn_save->Bind(wxEVT_LEAVE_WINDOW, [this](auto& event) {
            bool* sys_Value = static_cast<bool*>(m_btn_save->GetClientData());
            if (*sys_Value)
                return;
            m_btn_save->SetBackgroundColour(m_normal_color);
            });
        //m_btn_save->SetCornerRadius(3);
        //m_btn_save->SetBorderWidth(0);
        //m_btn_save->SetFontBold(true);
        //m_btn_save->SetMinSize({ (100) * em / 10, 30 * em / 10 });
        //m_btn_save->SetBackgroundColor(bg_color);
        //m_btn_save->SetTextColor(text_color);
        m_btn_save->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event) {
            if (!m_current_tab)
                return;

            Tab* cur_tab = dynamic_cast<Tab*> (m_current_tab);
            cur_tab->set_just_edit(true);
            cur_tab->save_preset(std::string(), false, false, false, std::string(), true);
            //OnPanelShowInit();
            cur_tab->set_just_edit(false);

            OnParentDialogOpen();
            Layout();
            });
        Slic3r::GUI::wxGetApp().UpdateDarkUI(m_btn_save);


        m_btn_saveAs = new wxButton(m_bottomBtnsPanel, wxID_ANY, _L("SaveAs"), wxDefaultPosition, wxDefaultSize,
            wxLeft | wxBORDER_NONE);
        m_btn_saveAs->SetMinSize({ FromDIP(100), FromDIP(30)});
        m_btn_saveAs->SetBackgroundColour(m_normal_color);
        m_btn_saveAs->SetClientData(new bool(false));
        m_btn_saveAs->SetFont(font);
        m_btn_saveAs->Bind(wxEVT_ENTER_WINDOW, [this](auto& event) {
            bool* sys_Value = static_cast<bool*>(m_btn_saveAs->GetClientData());
            if (*sys_Value)
                return;
            m_btn_saveAs->SetBackgroundColour(m_hover_color);
            });
        m_btn_saveAs->Bind(wxEVT_LEAVE_WINDOW, [this](auto& event) {
            bool* sys_Value = static_cast<bool*>(m_btn_saveAs->GetClientData());
            if (*sys_Value)
                return;
            m_btn_saveAs->SetBackgroundColour(m_normal_color);
            });

        m_btn_saveAs->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event) {
            if (!m_current_tab)
                return;
            Tab* cur_tab = dynamic_cast<Tab*> (m_current_tab);
            cur_tab->save_preset(std::string(), false, false, false, std::string(), false);

            OnParentDialogOpen();
            Layout();
            });
        Slic3r::GUI::wxGetApp().UpdateDarkUI(m_btn_saveAs);


        m_btn_delete = new wxButton(m_bottomBtnsPanel, wxID_ANY, _L("Delete"), wxDefaultPosition, wxDefaultSize,
            wxLeft | wxBORDER_NONE);
        m_btn_delete->SetMinSize({ FromDIP(100), FromDIP(30)});
        m_btn_delete->SetBackgroundColour(m_normal_color);
        m_btn_delete->SetClientData(new bool(false));
        m_btn_delete->SetFont(font);
        m_btn_delete->Bind(wxEVT_ENTER_WINDOW, [this](auto& event) {
            bool* sys_Value = static_cast<bool*>(m_btn_delete->GetClientData());
            if (*sys_Value)
                return;
            m_btn_delete->SetBackgroundColour(m_hover_color);
            });
        m_btn_delete->Bind(wxEVT_LEAVE_WINDOW, [this](auto& event) {
            bool* sys_Value = static_cast<bool*>(m_btn_delete->GetClientData());
            if (*sys_Value)
                return;
            m_btn_delete->SetBackgroundColour(m_normal_color);
            });

        m_btn_delete->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event) {
            if (!m_current_tab)
                return;

            Tab* cur_tab = dynamic_cast<Tab*> (m_current_tab);
            cur_tab->delete_preset();

            OnParentDialogOpen();
            //m_preset_listBox->DeleteItem();
            //OnPanelShowInit();
            });
        Slic3r::GUI::wxGetApp().UpdateDarkUI(m_btn_delete);


        m_btn_resets = new wxButton(m_bottomBtnsPanel, wxID_ANY, _L("Reset"), wxDefaultPosition, wxDefaultSize,
            wxLeft | wxBORDER_NONE);
        m_btn_resets->SetMinSize({ FromDIP(100), FromDIP(30)});
        m_btn_resets->SetBackgroundColour(m_normal_color);
        m_btn_resets->SetClientData(new bool(false));
        m_btn_resets->SetFont(font);
        m_btn_resets->Bind(wxEVT_ENTER_WINDOW, [this](auto& event) {
            bool* sys_Value = static_cast<bool*>(m_btn_resets->GetClientData());
            if (*sys_Value)
                return;
            m_btn_resets->SetBackgroundColour(m_hover_color);
            });
        m_btn_resets->Bind(wxEVT_LEAVE_WINDOW, [this](auto& event) {
            bool* sys_Value = static_cast<bool*>(m_btn_resets->GetClientData());
            if (*sys_Value)
                return;
            m_btn_resets->SetBackgroundColour(m_normal_color);
            });

        m_btn_resets->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event) {
            if (!m_current_tab)
                return;

            Tab* cur_tab = dynamic_cast<Tab*> (m_current_tab);
            cur_tab->on_roll_back_value();
            Layout();
            });
        Slic3r::GUI::wxGetApp().UpdateDarkUI(m_btn_resets);

        CusDialog* cd = new CusDialog(this);
        wxPanel* panel = cd->titlePanel();
        wxSizer* sizer = panel->GetSizer();

        RoundedPanel* rp = new RoundedPanel(panel);
        //rp->SetMinSize(wxSize(203, 31));

        CustomRoundCornerButton* rcb = new CustomRoundCornerButton(rp, wxID_ANY, _L("System"), wxDefaultPosition, wxSize(100, 30),
            wxLeft | wxBORDER_NONE);
        CustomRoundCornerButton* rcb1 = new CustomRoundCornerButton(rp, wxID_ANY, _L("User"), wxDefaultPosition, wxSize(100, 30),
            wxLeft | wxBORDER_NONE);

        rcb->SetBorderColor(wxColour("#6E6E72"));
        rcb1->SetBorderColor(wxColour("#6E6E72"));

        rcb->onClicked = [rcb1]() {
            rcb1->SetIsChecked(false); };
        rcb1->onClicked = [rcb]() {
            rcb->SetIsChecked(false); };

        wxSizer* rpSizer = new wxBoxSizer(wxHORIZONTAL);
        //rpSizer->AddStretchSpacer();
        rpSizer->Add(rcb, 0, wxDOWN, 0);
        rpSizer->Add(rcb1, 0, wxDOWN, 0);
        //rpSizer->AddStretchSpacer();

        //rp->SetSizer(rpSizer);
        rp->SetSizerAndFit(rpSizer);
        cd->addToolButton(rp);

        wxButton* openBtn = new wxButton(m_bottomBtnsPanel, wxID_ANY, _L("Open"), wxDefaultPosition, wxDefaultSize, wxLeft | wxBORDER_NONE);
        openBtn->Bind(wxEVT_BUTTON, [this, cd](wxCommandEvent& event) {
            cd->Popup();
            });
        openBtn->Hide();

        buttons_sizer_bt->AddStretchSpacer();
        buttons_sizer_bt->Add(m_btn_save);
        buttons_sizer_bt->AddSpacer(FromDIP(8));
        buttons_sizer_bt->Add(m_btn_saveAs);
        buttons_sizer_bt->AddSpacer(FromDIP(8));
        buttons_sizer_bt->Add(m_btn_delete);
        buttons_sizer_bt->AddSpacer(FromDIP(8));
        buttons_sizer_bt->Add(m_btn_resets);
        buttons_sizer_bt->AddSpacer(FromDIP(8));
        buttons_sizer_bt->Add(openBtn);
        buttons_sizer_bt->AddStretchSpacer();

        m_top_sizer->Add(m_bottomBtnsPanel, 0, wxALL | wxEXPAND | wxALIGN_CENTER_HORIZONTAL, 10);
    }

    //m_right_sizer = new wxBoxSizer( wxVERTICAL );

    //m_right_sizer->Add( m_page_view, 1, wxEXPAND | wxALL, 5 );

    //m_top_sizer->Add( m_right_sizer, 1, wxEXPAND, 5 );
    // BBS: new layout
    m_left_sizer->AddSpacer(6 * em_unit(this) / 10);
//#if __WXOSX__
//    m_left_sizer->Add(m_tmp_panel, 1, wxEXPAND | wxALL, 0);
//    m_tmp_panel->GetSizer()->Add(m_page_view, 1, wxEXPAND);
//#else
    // m_left_sizer->Add( m_page_view, 4, wxEXPAND );
    m_left_sizer->Add(m_color_border_box, 4, wxEXPAND | wxRIGHT, FromDIP(20));
//#endif

    if (m_img_tooltip_panel)
    {
        m_left_sizer->AddSpacer(FromDIP(20) * em_unit(this) / 10);
        m_left_sizer->Add(m_img_tooltip_panel, 1, wxEXPAND);

        m_btn_show_img_tooltip->SetMinSize(wxSize(50, 20));
        m_left_sizer->AddSpacer(FromDIP(15) * em_unit(this) / 10);
        m_left_sizer->Add(m_btn_show_img_tooltip, 0, wxALIGN_RIGHT | wxALL, 5);
        m_left_sizer->AddSpacer(FromDIP(5) * em_unit(this) / 10);

    }

    {
        wxBoxSizer* sizer2 = new wxBoxSizer(wxHORIZONTAL);
        sizer2->Add(m_button_save, 0, wxRIGHT, FromDIP(10));
        //sizer2->Add(m_button_save_as, 0, wxRIGHT, FromDIP(10));
        sizer2->Add(m_button_delete, 0, wxRIGHT, FromDIP(10));
        sizer2->Add(m_button_reset, 0, wxRIGHT, FromDIP(10));
        //m_left_sizer->Add(sizer2, 0, wxALIGN_RIGHT | wxRIGHT | wxBOTTOM);
        //m_left_sizer->AddSpacer(10 * em_unit(this) / 10);
    }

    //this->SetSizer( m_top_sizer );
    this->Layout();
}

void ParamsPanel::create_layout_process()
{
//#ifdef __WINDOWS__
//    this->SetDoubleBuffered(true);
//    m_page_view->SetDoubleBuffered(true);
//#endif //__WINDOWS__

    m_left_sizer = new wxBoxSizer(wxVERTICAL);
    // BBS: new layout
    m_left_sizer->SetMinSize(wxSize(FromDIP(40) * em_unit(this), -1));

    if (m_top_panel) {
        m_mode_sizer = new wxBoxSizer(wxHORIZONTAL);
        m_mode_sizer->AddSpacer(FromDIP(11));
        m_mode_sizer->Add(m_process_icon, 0, wxALIGN_CENTER);
        m_mode_sizer->AddSpacer(FromDIP(11));
        m_mode_sizer->Add(m_title_label, 0, wxALIGN_CENTER);
        m_mode_sizer->AddStretchSpacer(FromDIP(2));
        m_mode_sizer->Add(m_mode_region, 0, wxALIGN_CENTER);
        m_mode_sizer->AddStretchSpacer(FromDIP(1));
        m_mode_sizer->Add(m_tips_arrow, 0, wxALIGN_CENTER);
        m_mode_sizer->AddStretchSpacer(FromDIP(10));
        //         m_mode_sizer->Add( m_title_view, 0, wxALIGN_CENTER );
        //         m_mode_sizer->AddSpacer(FromDIP(2));

        m_mode_sizer->AddStretchSpacer(FromDIP(10));
        m_mode_sizer->Add(m_mode_view_box, 0, wxALIGN_CENTER);
        m_mode_sizer->AddSpacer(FromDIP(14));
        // m_mode_sizer->Add(m_btn_reset, 0, wxALIGN_CENTER);  // the m_btn_reset is not used
        // m_mode_sizer->AddSpacer(FromDIP(14));
        m_mode_sizer->Add(m_setting_btn, 0, wxALIGN_CENTER);
        m_mode_sizer->AddSpacer(FromDIP(14));
        m_mode_sizer->Add(m_compare_btn, 0, wxALIGN_CENTER);

        m_mode_sizer->AddSpacer(FromDIP(15));
        //m_mode_sizer->Add( m_search_btn, 0, wxALIGN_CENTER );
        //m_mode_sizer->AddSpacer(16);
        m_mode_sizer->SetMinSize(-1, FromDIP(35));
        m_top_panel->SetSizer(m_mode_sizer);
        //m_left_sizer->Add( m_top_panel, 0, wxEXPAND );
    }

    if (m_tab_print) {
        //m_print_sizer = new wxBoxSizer( wxHORIZONTAL );
        //m_print_sizer->Add( m_tab_print, 1, wxEXPAND | wxALL, 5 );
        //m_left_sizer->Add( m_print_sizer, 1, wxEXPAND, 5 );
        m_left_sizer->Add(m_tab_print, 0, wxEXPAND);
    }

    if (m_tab_print_plate) {
        m_left_sizer->Add(m_tab_print_plate, 0, wxEXPAND);
    }

    if (m_tab_print_object) {
        m_left_sizer->Add(m_tab_print_object, 0, wxEXPAND);
    }

    if (m_tab_print_part) {
        m_left_sizer->Add(m_tab_print_part, 0, wxEXPAND);
    }

    if (m_tab_print_layer) {
        m_left_sizer->Add(m_tab_print_layer, 0, wxEXPAND);
    }

    if (m_tab_filament) {
        //m_filament_sizer = new wxBoxSizer( wxVERTICAL );
        //m_filament_sizer->Add( m_tab_filament, 1, wxEXPAND | wxALL, 5 );
       // m_left_sizer->Add( m_filament_sizer, 1, wxEXPAND, 5 );
        //m_left_sizer->Add(m_tab_filament, 0, wxEXPAND);
    }

    if (m_tab_printer) {
        //m_printer_sizer = new wxBoxSizer( wxVERTICAL );
        //m_printer_sizer->Add( m_tab_printer, 1, wxEXPAND | wxALL, 5 );
        //m_left_sizer->Add(m_tab_printer, 0, wxEXPAND);
    }

    //m_left_sizer->Add( m_printer_sizer, 1, wxEXPAND, 1 );

    //m_button_sizer = new wxBoxSizer( wxHORIZONTAL );

    //m_button_sizer->Add( m_export_to_file, 0, wxALL, 5 );

    //m_button_sizer->Add( m_import_from_file, 0, wxALL, 5 );

    //m_left_sizer->Add( m_button_sizer, 0, wxALIGN_CENTER, 5 );

    {
        //m_top_sizer->AddSpacer(10 * em_unit(this) / 10);
        //m_top_sizer->Add(m_h_tabbar_printer, 0, wxEXPAND);

        wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
        m_top_sizer->Add(sizer, 1, wxEXPAND);


        //wxBoxSizer* sizer3 = new wxBoxSizer(wxVERTICAL);
        //sizer3->Add(m_left_sizer, 1, wxEXPAND);
        //
        //sizer3->AddSpacer(10 * em_unit(this) / 10);

        //sizer->AddSpacer(10 * em_unit(this) / 10);
        //sizer->Add(m_v_tabbar_printer, 4, wxEXPAND);
        sizer->Add(m_left_sizer, 9, wxEXPAND);
    }

    //m_right_sizer = new wxBoxSizer( wxVERTICAL );

    //m_right_sizer->Add( m_page_view, 1, wxEXPAND | wxALL, 5 );

    //m_top_sizer->Add( m_right_sizer, 1, wxEXPAND, 5 );
    // BBS: new layout
    m_left_sizer->AddSpacer(FromDIP(6) * em_unit(this) / 10);
//#if __WXOSX__
//    m_left_sizer->Add(m_tmp_panel, 1, wxEXPAND | wxALL, 0);
//    m_tmp_panel->GetSizer()->Add(m_page_view, 1, wxEXPAND);
//#else
    // m_left_sizer->Add( m_page_view, 4, wxEXPAND );
    m_left_sizer->Add(m_color_border_box, 4, wxEXPAND);
//#endif

    if (m_img_tooltip_panel)
    {
        m_left_sizer->AddSpacer(FromDIP(20) * em_unit(this) / 10);
        m_left_sizer->Add(m_img_tooltip_panel, 1, wxEXPAND);

        m_btn_show_img_tooltip->SetMinSize(wxSize(50, 20));
        m_left_sizer->AddSpacer(FromDIP(15) * em_unit(this) / 10);
        m_left_sizer->Add(m_btn_show_img_tooltip, 0, wxALIGN_RIGHT | wxALL, 5);
        m_left_sizer->AddSpacer(FromDIP(5) * em_unit(this) / 10);

    }

    {
        wxBoxSizer* sizer2 = new wxBoxSizer(wxHORIZONTAL);
        //sizer2->Add(m_button_save, 0, wxRIGHT, FromDIP(10));
        ////sizer2->Add(m_button_save_as, 0, wxRIGHT, FromDIP(10));
        //sizer2->Add(m_button_delete, 0, wxRIGHT, FromDIP(10));
        //sizer2->Add(m_button_reset, 0, wxRIGHT, FromDIP(10));
        //m_left_sizer->Add(sizer2, 0, wxALIGN_RIGHT | wxRIGHT | wxBOTTOM);
        //m_left_sizer->AddSpacer(10 * em_unit(this) / 10);
    }

    //this->SetSizer( m_top_sizer );
    this->Layout();
}

void ParamsPanel::rebuild_panels()
{
    refresh_tabs();
    free_sizers();
    create_layout();
}

void ParamsPanel::refresh_tabs()
{
    auto& tabs_list = wxGetApp().tabs_list;
    auto print_tech = wxGetApp().preset_bundle->printers.get_selected_preset().printer_technology();
    for (auto tab : tabs_list)
        if (tab->supports_printer_technology(print_tech))
        {
            if (tab->GetParent() != this) continue;
            switch (tab->type())
            {
                case Preset::TYPE_PRINT:
                case Preset::TYPE_SLA_PRINT:
                    m_tab_print = tab;
                    break;

                case Preset::TYPE_FILAMENT:
                case Preset::TYPE_SLA_MATERIAL:
                    m_tab_filament = tab;
                    break;

                case Preset::TYPE_PRINTER:
                    m_tab_printer = tab;
                    break;
                default:
                    break;
            }
        }
    if (m_top_panel) {
        m_tab_print_plate = wxGetApp().get_plate_tab();
        m_tab_print_object = wxGetApp().get_model_tab();
        m_tab_print_part = wxGetApp().get_model_tab(true);
        m_tab_print_layer = wxGetApp().get_layer_tab();
    }
    return;
}

void ParamsPanel::clear_page()
{
    if (m_page_sizer)
        m_page_sizer->Clear(true);
}


void ParamsPanel::OnActivate()
{
    if (m_current_tab == NULL)
    {
        //the first time
        BOOST_LOG_TRIVIAL(debug) << __FUNCTION__ << boost::format(": first time opened, set current tab to print");
        // BBS: open/close tab
        //m_current_tab = m_tab_print;
        set_active_tab(m_tab_print ? m_tab_print : m_tab_filament);
    }
    Tab* cur_tab = dynamic_cast<Tab *> (m_current_tab);
    if (cur_tab)
        cur_tab->OnActivate();
}

void ParamsPanel::OnToggled(wxCommandEvent& event)
{
    if (m_mode_region && m_mode_region->GetId() == event.GetId()) {
        wxWindowUpdateLocker locker(GetParent());
        set_active_tab(nullptr);
        event.Skip();
        return;
    }

    if (wxID_ABOUT != event.GetId()) {
        return;
    }

    // this is from tab's mode switch
    bool value = dynamic_cast<SwitchButton*>(event.GetEventObject())->GetValue();
    int mode_id;

    BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << boost::format(": Advanced mode toogle to %1%") % value;

    if (value)
    {
        //m_mode_region->SetBitmap(m_toggle_on_icon);
        mode_id = comAdvanced;
    }
    else
    {
        //m_mode_region->SetBitmap(m_toggle_off_icon);
        mode_id = comSimple;
    }

    Slic3r::GUI::wxGetApp().save_mode(mode_id);

    if (m_mode_view && m_mode_view->GetId() == event.GetId()) {
        m_mode_view->updateBitmapHover();
    }
}

void ParamsPanel::OnToggledImageTooltip(wxCommandEvent& event)
{
    if(m_img_tooltip_panel)
        m_img_tooltip_panel->Show();

    if(m_img_tooltip_sizer)
        m_img_tooltip_sizer->Show(true);

    if(m_btn_show_img_tooltip)
        m_btn_show_img_tooltip->Hide();

    if(m_left_sizer) {
        m_left_sizer->Layout();
    }
}

// This is special, DO NOT call it from outer except from Tab
void ParamsPanel::set_active_tab(wxPanel* tab)
{
    Tab* cur_tab = dynamic_cast<Tab *> (tab);

    if (cur_tab == nullptr) {
        if (!m_mode_region->GetValue()) {
            cur_tab = (Tab*) m_tab_print;
        } else if (m_tab_print_part && ((TabPrintModel*) m_tab_print_part)->has_model_config()) {
            cur_tab = (Tab*) m_tab_print_part;
        } else if (m_tab_print_layer && ((TabPrintModel*)m_tab_print_layer)->has_model_config()) {
            cur_tab = (Tab*)m_tab_print_layer;
        } else if (m_tab_print_object && ((TabPrintModel*) m_tab_print_object)->has_model_config()) {
            cur_tab = (Tab*) m_tab_print_object;
        } else if (m_tab_print_plate && ((TabPrintPlate*)m_tab_print_plate)->has_model_config()) {
            cur_tab = (Tab*)m_tab_print_plate;
        }
        Show(cur_tab != nullptr);
        // wxGetApp().sidebar().show_object_list(m_mode_region->GetValue());  // the object_list ui has been relayouted
        wxGetApp().plater()->get_process_toolbar().on_params_panel_tab_changed(cur_tab);
        if (m_current_tab == cur_tab)
            return;
        if (cur_tab)
            cur_tab->restore_last_select_item();
        return;
    }

    m_current_tab = tab;
    on_active_tab_changed();
    BOOST_LOG_TRIVIAL(debug) << __FUNCTION__ << boost::format(": set current to %1%, type=%2%") % cur_tab % cur_tab?cur_tab->type():-1;
    update_mode();

    // BBS: open/close tab
    for (auto t : std::vector<std::pair<wxPanel*, wxStaticLine*>>({
            {m_tab_print, m_staticline_print},
            {m_tab_print_object, m_staticline_print_object},
            {m_tab_print_part, m_staticline_print_part},
            {m_tab_print_layer, nullptr},
            {m_tab_print_plate, nullptr},
            {m_tab_filament, m_staticline_filament},
            {m_tab_printer, m_staticline_printer}})) {
        if (!t.first) continue;
        t.first->Show(tab == t.first);
   /*     m_h_tabbar_printer->Show(tab == m_tab_printer || tab == m_tab_filament);
        m_v_tabbar_printer->Show(tab == m_tab_printer || tab == m_tab_filament);*/
        m_button_save->Show(tab == m_tab_printer || tab == m_tab_filament);
        m_button_save->Show(false);
        //m_button_save_as->Show(tab == m_tab_printer || tab == m_tab_filament);
        m_button_delete->Show((tab == m_tab_printer || tab == m_tab_filament) && cur_tab->m_presets->get_edited_preset().is_user());
        m_button_delete->Show(false);
        m_button_reset->Show(tab == m_tab_printer || tab == m_tab_filament);
        m_button_reset->Show(false);
        if (!t.second) continue;
        t.second->Show(tab == t.first);
        //m_left_sizer->GetItem(t)->SetProportion(tab == t ? 1 : 0);
    }
    m_left_sizer->Layout();
    if (auto dialog = dynamic_cast<wxDialog*>(GetParent())) {
        wxString title = cur_tab->type() == Preset::TYPE_FILAMENT ? _L("Filament settings") : _L("Printer settings");
        dialog->SetTitle(title);
    }

    auto tab_print = dynamic_cast<Tab *>(m_tab_print);
    if (cur_tab == m_tab_print) {
        if (tab_print)
            tab_print->toggle_line("print_flow_ratio", false);
    } else {
        if (tab_print)
            tab_print->toggle_line("print_flow_ratio", false);
    }
}

bool ParamsPanel::is_active_and_shown_tab(wxPanel* tab)
{
    if (m_current_tab == tab)
        return true;
    else
        return false;
}

void ParamsPanel::on_active_tab_changed()
{
    auto&                      processToolBar = wxGetApp().plater()->get_process_toolbar();
    ProcessBar::GLToolbarItem* frequency_item = processToolBar.get_item(L("Frequent"), false);
    if (frequency_item && m_mode_region) {
        frequency_item->set_visible(m_mode_region->GetValue());
        processToolBar.set_layout_dirty();
    }

    processToolBar.reset_all_items_state_to_normal();

    TabPrint* tab_print = dynamic_cast<TabPrint*>(m_current_tab);
    if (tab_print != nullptr) {
        Page* active_page = tab_print->get_active_page();
        if (active_page != nullptr) {
            wxString                   page_title = active_page->title();
            if(page_title != L("Plate Settings")) {
                ProcessBar::GLToolbarItem* title_item = processToolBar.get_item(page_title.ToStdString());
                if (title_item) {
                    title_item->set_state(ProcessBar::GLToolbarItem::Pressed);
                }
            }
        }
    }
}

bool ParamsPanel::current_tab_is_plate_settings()
{
    TabPrint* tab_print = dynamic_cast<TabPrint*>(m_current_tab);
    if (tab_print != nullptr) {
        Page* active_page = tab_print->get_active_page();
        if (active_page != nullptr) {
            wxString                   page_title = active_page->title();
            if(page_title == L("Plate Settings")) {
                return true;
            }
        }
    }

    return false;

}

void ParamsPanel::create_image_tooltip_panel()
{
    m_img_tooltip_panel = new ImageTooltipPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize);

    m_img_tooltip_sizer = new wxBoxSizer( wxHORIZONTAL);
    m_img_tooltip_sizer->AddSpacer(FromDIP(10));
    m_img_tooltip_sizer->Add(m_img_tooltip_panel, 1, wxALIGN_CENTER, 0);
    m_img_tooltip_sizer->AddSpacer(FromDIP(10));

    m_btn_show_img_tooltip = new ScalableButton(this, wxID_ANY, wxGetApp().dark_mode() ? "button_show_img_tooltip" : "button_show_img_light",
                                                wxEmptyString, wxDefaultSize, wxDefaultPosition, wxBU_EXACTFIT | wxNO_BORDER, true, 10,
                                                true);
    m_btn_show_img_tooltip->Bind(wxEVT_BUTTON, &ParamsPanel::OnToggledImageTooltip, this);
    m_btn_show_img_tooltip->Hide();
}

void ParamsPanel::set_border_panel_color(const wxColour& border_color)
{
    StateColor border_outline_bg(std::pair<wxColour, int>(border_color, StateColor::Enabled),
                                 std::pair<wxColour, int>(border_color, StateColor::Normal));
    if(m_color_border_box)
    {
        m_color_border_box->SetBorderWidth(FromDIP(1));
        m_color_border_box->SetBorderColor(border_outline_bg);
    }
}

void ParamsPanel::update_mode(const int mode)
{
    int app_mode = mode;
    if (mode == -1) {
        app_mode = Slic3r::GUI::wxGetApp().get_mode();
    }
    SwitchButton * mode_view = m_current_tab ? dynamic_cast<Tab*>(m_current_tab)->m_mode_view : nullptr;
    if (mode_view == nullptr) mode_view = m_mode_view;
    if (mode_view == nullptr) return;

    if (!Slic3r::GUI::wxGetApp().app_config->has("user_mode")) {
        if (wxGetApp().app_config->get("role_type") == "0") {
            mode_view->SetValue(false);
        } else {
            mode_view->SetValue(true);
        }
        return;
    }
    //BBS: disable the mode tab and return directly when enable develop mode
    if (app_mode == comDevelop)
    {
        mode_view->Disable();
        return;
    }
    if (!mode_view->IsEnabled())
        mode_view->Enable();

    if (app_mode == comAdvanced)
    {
        mode_view->SetValue(true);
    }
    else
    {
        mode_view->SetValue(false);
    }
}

void ParamsPanel::msw_rescale()
{
    if (m_process_icon) m_process_icon->msw_rescale();
    if (m_setting_btn) m_setting_btn->msw_rescale();
    if (m_search_btn) m_search_btn->msw_rescale();
    if (m_compare_btn) m_compare_btn->msw_rescale();
    if (m_tips_arrow) m_tips_arrow->msw_rescale();
    if (m_left_sizer) m_left_sizer->SetMinSize(wxSize(FromDIP(40) * em_unit(this), -1));
    if (m_mode_sizer)
        m_mode_sizer->SetMinSize(-1, FromDIP(3) * em_unit(this));
    if (m_mode_region)
        ((SwitchButton* )m_mode_region)->Rescale();
    if (m_mode_view)
        ((SwitchButton* )m_mode_view)->Rescale();
    for (auto tab : {m_tab_print, m_tab_print_plate, m_tab_print_object, m_tab_print_part, m_tab_print_layer, m_tab_filament, m_tab_printer}) {
        if (tab) dynamic_cast<Tab*>(tab)->msw_rescale();
    }
    //((Button*)m_export_to_file)->Rescale();
    //((Button*)m_import_from_file)->Rescale();
}

void ParamsPanel::switch_to_global()
{
    m_mode_region->SetValue(false);
    set_active_tab(nullptr);
}

void ParamsPanel::switch_to_object(bool with_tips)
{
    m_mode_region->SetValue(true);
    set_active_tab(nullptr);
    if (with_tips) {
        m_highlighter.init(std::pair(m_tips_arrow, &m_tips_arror_blink), m_top_panel);
        m_highlighter.blink();
    }
}

bool ParamsPanel::get_switch_of_object()
{
    if (m_mode_region) {
        return m_mode_region->GetValue();
    }
    return false;
}

void ParamsPanel::notify_object_config_changed()
{
    this->notify_config_changed();

    auto & model = wxGetApp().model();
    bool has_config = false;
    for (auto obj : model.objects) {
        if (!obj->config.empty()) {
            SettingsFactory::Bundle cat_options = SettingsFactory::get_bundle(&obj->config.get(), true);
            if (cat_options.size() > 0) {
                has_config = true;
                break;
            }
        }
        for (auto volume : obj->volumes) {
            if (!volume->config.empty()) {
                SettingsFactory::Bundle cat_options = SettingsFactory::get_bundle(&volume->config.get(), true);
                if (cat_options.size() > 0) {
                    has_config = true;
                    break;
                }
            }
        }
        if (has_config) break;
    }
    if (has_config == m_has_object_config) return;
    m_has_object_config = has_config;
    if (has_config)
        m_mode_region->SetTextColor2(StateColor(std::pair{0xfffffe, (int) StateColor::Checked}, std::pair{wxGetApp().get_label_clr_modified(), 0}));
    else
        m_mode_region->SetTextColor2(StateColor());
    m_mode_region->Rescale();
}

void ParamsPanel::notify_config_changed()
{
    auto& model      = wxGetApp().model();
    bool  is_dirt    = false;
    bool  has_config = false;
    for (auto obj : model.objects) {
        if (!obj->config.empty()) {
            SettingsFactory::Bundle cat_options = SettingsFactory::get_bundle(&obj->config.get(), true);
            if (cat_options.size() > 0) {
                has_config = true;
                break;
            }
        }
        for (auto volume : obj->volumes) {
            if (!volume->config.empty()) {
                SettingsFactory::Bundle cat_options = SettingsFactory::get_bundle(&volume->config.get(), true);
                if (cat_options.size() > 0) {
                    has_config = true;
                    break;
                }
            }
        }
        if (has_config)
            break;
    }

    if (has_config)//
        is_dirt = true;

    if (!is_dirt)
    {
        Tab* pTab = dynamic_cast<Tab*>(m_current_tab);
        if (nullptr != pTab) {
            PresetCollection* preset = pTab->get_presets();
            if (nullptr != preset) {
                is_dirt = preset->get_edited_preset().is_dirty;
            }
        }
    }

    // m_btn_reset->Enable(is_dirt);  // no need this reset button
}

void ParamsPanel::switch_to_object_if_has_object_configs()
{
    if (m_has_object_config)
        m_mode_region->SetValue(true);
    set_active_tab(nullptr);
}

void ParamsPanel::sys_color_changed()
{
    bool is_dark = wxGetApp().dark_mode();
    m_process_icon->SetBitmap_(is_dark ? "process_dark_default" : "process_light_default");
  //  m_btn_reset->SetBitmap_(is_dark ? "dot" : "undo");    
    //m_setting_btn->SetBitmap_(is_dark ? "table" : "table"); 

    m_mode_view->GetOnImg() = ScalableBitmap(m_mode_view, is_dark ? "advanced_process_dark_checked" : "advanced_process_light_checked", FromDIP(14));
    m_mode_view->GetOffImg() = ScalableBitmap(m_mode_view, is_dark ? "advanced_process_dark" : "advanced_process_light", FromDIP(14));

    m_compare_btn->SetBitmap_(is_dark ? "compare_dark_default" : "compare_light_default");

    m_compare_btn->on_sys_color_changed(is_dark);
    m_setting_btn->on_sys_color_changed(is_dark);

}

void ParamsPanel::show_image_tooltip(wxString text_tooltip,wxString img_path)
{
    if(!m_img_tooltip_panel || !m_img_tooltip_panel->IsShown())
        return;

    m_img_tooltip_panel->SetBackgroundImage(img_path,text_tooltip);

}

void ParamsPanel::on_change_color_mode(bool is_dark)
{
    if(m_img_tooltip_panel)
        m_img_tooltip_panel->on_change_color_mode(is_dark);

    if(m_btn_show_img_tooltip) {
            wxBitmap btn_bitmap = create_scaled_bitmap(is_dark ? "button_show_img_tooltip" : "button_show_img_light", this, 10);
            m_btn_show_img_tooltip->SetBitmap(btn_bitmap);
    }

    if(m_mode_view_box)
    {

    }

}

void ParamsPanel::free_sizers()
{
    if (m_top_sizer)
    {
        m_top_sizer->Clear(false);
        //m_top_sizer = nullptr;
    }

    m_left_sizer = nullptr;
    //m_right_sizer = nullptr;
    m_mode_sizer = nullptr;
    //m_print_sizer = nullptr;
    //m_filament_sizer = nullptr;
    //m_printer_sizer = nullptr;
    m_button_sizer = nullptr;
}

void ParamsPanel::tab_page_relayout()
{
    if(m_img_tooltip_panel)
        m_img_tooltip_panel->Hide();

    if(m_img_tooltip_sizer)
        m_img_tooltip_sizer->Show(false);

    if(m_btn_show_img_tooltip)
        m_btn_show_img_tooltip->Show();

    if(m_left_sizer) {
        m_left_sizer->Layout();
    }

}

void ParamsPanel::delete_subwindows()
{
    if (m_title_label)
    {
        delete m_title_label;
        m_title_label = nullptr;
    }

    if (m_mode_region)
    {
        delete m_mode_region;
        m_mode_region = nullptr;
    }

    if (m_mode_view)
    {
        delete m_mode_view;
        m_mode_view = nullptr;
    }

    if(m_mode_view_box)
    {
        delete m_mode_view_box;
        m_mode_view_box = nullptr;
    }

//     if (m_title_view)
//     {
//         delete m_title_view;
//         m_title_view = nullptr;
//     }

    if (m_search_btn)
    {
        delete m_search_btn;
        m_search_btn = nullptr;
    }

    if (m_staticline_print)
    {
        delete m_staticline_print;
        m_staticline_print = nullptr;
    }

    if (m_staticline_print_part)
    {
        delete m_staticline_print_part;
        m_staticline_print_part = nullptr;
    }

    if (m_staticline_print_object)
    {
        delete m_staticline_print_object;
        m_staticline_print_object = nullptr;
    }

    if (m_staticline_filament)
    {
        delete m_staticline_filament;
        m_staticline_filament = nullptr;
    }

    if (m_staticline_printer)
    {
        delete m_staticline_printer;
        m_staticline_printer = nullptr;
    }

    if (m_export_to_file)
    {
        delete m_export_to_file;
        m_export_to_file = nullptr;
    }

    if (m_import_from_file)
    {
        delete m_import_from_file;
        m_import_from_file = nullptr;
    }

    if (m_page_view)
    {
        delete m_page_view;
        m_page_view = nullptr;
    }

    if(m_img_tooltip_panel)
    {
        delete m_img_tooltip_panel;
        m_img_tooltip_panel = nullptr;
    }

    if(m_btn_show_img_tooltip)
    {
        delete m_btn_show_img_tooltip;
        m_btn_show_img_tooltip = nullptr;
    }

    if(m_img_tooltip_sizer)
    {
        delete m_img_tooltip_sizer;
        m_img_tooltip_sizer = nullptr;
    }

    if(m_color_border_box)
    {
        delete m_color_border_box;      
        m_color_border_box = nullptr;
    }
}
class MyRenderer : public wxDataViewCustomRenderer
{
public:
    MyRenderer(const wxString& varianttype = GetDefaultType(),
        wxDataViewCellMode mode = wxDATAVIEW_CELL_INERT,
        int align = wxDVR_DEFAULT_ALIGNMENT)
        : wxDataViewCustomRenderer(varianttype, mode, align) {}

    virtual bool Render(wxRect rect, wxDC* dc, int state) override
    {
        wxString value;
        if (GetValueAsString(&value))
        {
            if (value == "Special")
            {
                dc->SetBrush(*wxRED_BRUSH);
                dc->SetPen(*wxTRANSPARENT_PEN);
                dc->DrawRectangle(rect);
                dc->SetTextForeground(*wxWHITE);
            }
            else
            {
                dc->SetBrush(*wxWHITE_BRUSH);
                dc->SetPen(*wxTRANSPARENT_PEN);
                dc->DrawRectangle(rect);
                dc->SetTextForeground(*wxBLACK);
            }

            dc->DrawText(value, rect.GetLeft() + 5, rect.GetTop() + 5);
        }
        return true;
    }

    virtual bool GetValueAsString(wxString* value) const
    {
        // 这里需要从数据模型中获取当前项的值
        // 假设我们有一个简单的数据模型，直接返回一个固定的值
        *value = "Normal"; // 你可以根据实际情况从数据模型中获取值
        return true;
    }
    virtual bool SetValue(const wxVariant& value) override
    {
        // 如果需要支持设置值，可以在这里实现
        return true;
    }

    virtual wxSize GetSize() const override
    {
        return wxSize(100, 20); // 返回渲染器的大小
    }

    virtual bool GetValue(wxVariant& value) const
    {
        value = "text";
        return true;
    }

    virtual bool SetValueFromString(const wxString& value)
    {
        // 如果需要支持设置值，可以在这里实现
        return true;
    }
};

ParamsPanel::~ParamsPanel()
{
#if 0
    free_sizers();
    delete m_top_sizer;

    delete_subwindows();
#endif
    // BBS: fix double destruct of OG_CustomCtrl
    Tab* cur_tab = dynamic_cast<Tab*> (m_current_tab);
    if (cur_tab)
        cur_tab->clear_pages();
}

void ParamsPanel::ShowDetialView(bool show)
{
    m_left_sizer->Show(show);
    if (show)
    {
        set_active_tab(m_current_tab);
    }
}

void ParamsPanel::ShowDeleteButton(bool show)
{
    //m_button_delete->Show(show);
    //Layout();
}

void ParamsPanel::OnPanelShowInit()
{
    Tab* tab = dynamic_cast<Tab*>(get_current_tab());
    if (!tab)
        return;

    Preset& preset = tab->get_presets()->get_edited_preset();

    m_tabbar_ctrl = nullptr;
    if (tab->type() == Preset::Type::TYPE_PRINTER)
    {
        m_ws = WS_PRINTER;
        m_tabbar_ctrl = m_tabbar_printer;
        m_filament_tabCtrl->Hide();
        m_printer_tabCtrl->Show();
       
        m_tab_printer->Show();
        m_tab_filament->Hide();
    }
    else if (tab->type() == Preset::Type::TYPE_FILAMENT)
    {
        m_ws = WS_FILAMENT;
        m_tabbar_ctrl = m_tabbar_filament;
        m_printer_tabCtrl->Hide();
        m_filament_tabCtrl->Show();

        m_tab_printer->Hide();
        m_tab_filament->Show();
    }

    if (m_tabbar_ctrl)
        m_tabbar_ctrl->OnInit(preset);

    OnParentDialogOpen();

    //initData();
    Refresh();
    Layout();
}

void ParamsPanel::OnPanelShowExit()
{
    Tab* tab = dynamic_cast<Tab*>(get_current_tab());
    if (!tab)
        return;

    if (m_tabbar_ctrl)
        m_tabbar_ctrl->OnExit();

    m_tabbar_ctrl = nullptr;
}

void ParamsPanel::OnParentDialogOpen()
{
    Preset preset = m_ws == WS_PRINTER ? wxGetApp().preset_bundle->printers.get_selected_preset() 
        : wxGetApp().preset_bundle->filaments.get_selected_preset();

    std::string name = from_u8(preset.label(false)).ToStdString();
    bool user = false;

    if (preset.is_default || preset.is_system)
    {
        user = false;
        m_ps = PS_SYSTEM;
    }
    else if (preset.is_project_embedded)
    {
        user = true;
        m_ps = PS_USER;
    }
    else {
        user = true;
        m_ps = PS_USER;
    }

    m_printerType = _L("ALL").ToStdString();
    m_curVentor = _L("ALL").ToStdString(); //getVendor(preset);
    m_curPreset = from_u8(preset.label(true));
    
    filteredData(m_curVentor, m_printerType);
    updateItemState();
}

void ParamsPanel::updateItemState()
{
    if (!m_btnsPanel)
        return;

    bool isSys = false;
    std::string res = wxGetApp().app_config->get("role_type");
    if (m_ws == WS_PRINTER)
    {
        if (!m_system_print_list.size() && !m_user_print_list.size() && !m_project_print_list.size())
        {
            m_tab_printer->Hide();
            m_left_sizer->Show(false);
            m_bottomBtnsPanel->Show(false);
        }
        else
        {
            m_tab_printer->Show();
            m_left_sizer->Show(true);
            m_bottomBtnsPanel->Show(true);
        }

        if (res == "0")
        {
            m_btnsPanel->Hide();
            dynamic_cast<TabPrinter*>(m_tab_printer)->m_boxPanel->Hide();
        }
        else
        {
            m_btnsPanel->Show();
            dynamic_cast<TabPrinter*>(m_tab_printer)->m_boxPanel->Show();
        }

        Preset preset = wxGetApp().preset_bundle->printers.get_selected_preset();
        if (preset.is_default || preset.is_system)
            isSys = true;
    }
    else if (m_ws == WS_FILAMENT)
    {
        if (!m_system_print_list.size() && !m_user_print_list.size() && !m_project_print_list.size())
        {
            m_tab_filament->Hide();
            m_left_sizer->Show(false);
            m_bottomBtnsPanel->Show(false);
        }
        else
        {
            m_tab_filament->Show();
            m_left_sizer->Show(true);
            m_bottomBtnsPanel->Show(true);
        }

        if (res == "0")
        {
            m_btnsPanel->Hide();
            dynamic_cast<TabFilament*>(m_tab_filament)->m_boxPanel->Hide();
        }
        else
        {
            m_btnsPanel->Show();
            dynamic_cast<TabFilament*>(m_tab_filament)->m_boxPanel->Show();
        }

        Preset preset = wxGetApp().preset_bundle->filaments.get_selected_preset();
        if (preset.is_default || preset.is_system)
            isSys = true;
    }
    

    function setSystemType = [this]() {
        m_btn_save->Hide();
        m_btn_delete->Hide();

        bool* sys_Value = static_cast<bool*>(m_btn_system->GetClientData());
        *sys_Value = true;
        bool* user_Value = static_cast<bool*>(m_btn_user->GetClientData());
        *user_Value = false;
        m_btn_user->SetBackgroundColour(wxColour(214, 214, 220));
        m_btn_system->SetBackgroundColour(wxColour(21, 191, 89));
    };

    function setUserType = [this]() {
        m_btn_save->Show();
        m_btn_delete->Show();

        bool* sys_Value = static_cast<bool*>(m_btn_system->GetClientData());
        *sys_Value = false;
        bool* user_Value = static_cast<bool*>(m_btn_user->GetClientData());
        *user_Value = true;
        m_btn_system->SetBackgroundColour(wxColour(214, 214, 220));
        m_btn_user->SetBackgroundColour(wxColour(21, 191, 89));
    };

    if (m_ps == PS_SYSTEM || (m_ps == PS_BASE && isSys))
    {
        setSystemType();
    }
    else if (m_ps == PS_USER)
    {
        setUserType();
    }
    else if (m_ps == PS_BASE)
    {
        if (isSys)
            setSystemType();
        else
            setUserType();
    }

    //选中状态
    Layout();
}

void ParamsPanel::onChangedSysAndUser(PageState ps)
{
    if (m_ps == ps)
        return;
    m_ps = ps;
    std::vector<wxString> system_print_list;
    std::vector<wxString> user_print_list;
    std::vector<wxString> project_print_list;
    std::unordered_set<wxString>  print_list;
    std::unordered_set<wxString>  ventor_list;

    getDatas(system_print_list, user_print_list, project_print_list, print_list, ventor_list);
    updateVentors(ventor_list);
    updatePrinters(print_list);
    updatePresetsList(user_print_list, system_print_list, project_print_list);
}

void ParamsPanel::onChangedVentor(std::string ventor)
{

}

void ParamsPanel::onChangedPrinterType(std::string printer)
{

}

void ParamsPanel::initData()
{
    //PresetCollection* presetCollection = &(wxGetApp().preset_bundle->filaments);
    //const std::deque<Preset>& presets = presetCollection->get_presets();

    //std::vector<Preset> nonsys_presets;
    //std::vector<Preset> project_embedded_presets;
    //std::vector<Preset> system_presets;
    //std::vector<Preset> preset_descriptions;

    std::deque<Preset> presets;
    size_t count;
    size_t idx_selected;
    if (m_ws == WS_PRINTER)
    {
        count = wxGetApp().preset_bundle->printers.num_default_presets();
        idx_selected = wxGetApp().preset_bundle->printers.get_selected_idx();
        presets = wxGetApp().preset_bundle->printers.get_presets();
    }
    else if(m_ws == WS_FILAMENT)
    {
        count = wxGetApp().preset_bundle->printers.num_default_presets();
        idx_selected = wxGetApp().preset_bundle->printers.get_selected_idx();
        presets = wxGetApp().preset_bundle->filaments.get_presets();
    }

    std::string presetType = std::string();
    std::string ventor = std::string();
    std::string presetName = std::string();
    std::multimap<std::string, Preset> sysPresets;
    std::multimap<std::string, Preset> userPresets;
    for (size_t i = presets.front().is_visible ? 0 : count; i < presets.size(); ++i)
    {
        Preset preset = presets[i];
        if (!preset.is_visible || (!preset.is_compatible && i != idx_selected))
            continue;

        ventor = getVendor(preset);
        presetType = getPresetType(preset);
        presetName = from_u8(preset.label(false)).ToStdString();

        if (preset.is_default || preset.is_system)
        {
            if (presetName != "")
            {
                sysPresets.insert(std::make_pair(presetType, preset));
                m_systemDatas.insert(std::make_pair(ventor, sysPresets));
            }  
        }
        else if (preset.is_project_embedded)
        {

        }
        else {
            if (presetName != "")
            {
                std::multimap<std::string, Preset> presets;
                presets.insert(std::make_pair(presetType, preset));
                m_userDatas.insert(std::make_pair(ventor, presets));
            } 
        }
    }
}

void ParamsPanel::filteredData(wxString choseVentor, wxString chosePrinterType, int changeType/* = 0*/)
{
    m_print_list.clear();
    m_ventor_list.clear();
    m_system_print_list.clear();
    m_user_print_list.clear();
    m_project_print_list.clear();

    getDatas(m_system_print_list, m_user_print_list, m_project_print_list, m_print_list, m_ventor_list);

    switch (changeType)
    {
        case 0:
        {
            updateVentors(m_ventor_list);
            updatePrinters(m_print_list);
            updatePresetsList(m_system_print_list, m_user_print_list, m_project_print_list);
            break;
        }
        case 1:
        {
            //updateVentors(m_ventor_list);
            updatePrinters(m_print_list);
            updatePresetsList(m_system_print_list, m_user_print_list, m_project_print_list);
            break;
        }
        case 2:
        {
            //updateVentors(m_ventor_list);
            //updatePrinters(m_print_list);
            updatePresetsList(m_system_print_list, m_user_print_list, m_project_print_list);
            break;
        }
        case 3:
        {
            //updateVentors(ventor_list);
            //updatePrinters(print_list);
            //updatePresetsList(m_system_print_list, m_user_print_list);
            break;
        }
        default:
        {}
    }

}
std::string ParamsPanel::findInherits(const std::string& name, Preset::Type type)
{
    Preset* p = NULL;
    if (type == Preset::TYPE_FILAMENT) {
        p = wxGetApp().preset_bundle->filaments.find_preset(name);
    }
    else if (type == Preset::TYPE_PRINTER) {
        p = wxGetApp().preset_bundle->printers.find_preset(name);
    }

    if (!p)
        return "Custom";

    if (p->vendor)
        return p->vendor->name;

    const ConfigOptionVectorBase* vec = static_cast<const ConfigOptionVectorBase*>(p->config.option("filament_vendor"));
    if (!vec) {
        auto inherits = p->inherits();
        if (!inherits.empty())
            return findInherits(inherits, type);
        else
            return "Custom";
    }

    if (vec->is_vector()) {
        auto vendors = vec->vserialize();
        if (vendors.empty())
            return "Custom";
        else
            return vendors[0];
    }
    else
        return vec->serialize();
}


std::string ParamsPanel::getFilamentVendor(const Preset& preset)
{
    //if (preset.vendor)
    //    return preset.vendor->name;

    const ConfigOptionVectorBase* vec = static_cast<const ConfigOptionVectorBase*>(preset.config.option("filament_vendor"));
    if (!vec) {
        auto inherits = preset.inherits();
        if (!inherits.empty())
            return findInherits(inherits, preset.type);
        else
            return "Custom";
    }

    if (vec->is_vector())
    {
        auto vendors = vec->vserialize();
        if (vendors.empty())
            return "Custom";
        else
            return vendors[0];
    }
    else
        return vec->serialize();
}
std::string ParamsPanel::getVendor(const Preset& preset)
{
    if (m_ws == WS_PRINTER)
    {
        if (preset.vendor)
            return preset.vendor->name;

        std::string name = preset.config.opt_string("inherits");
        const std::map<std::string, ProfilePrinter*>& profile_printers = wxGetApp().app_config->get_profile_printers();
        auto iter = profile_printers.find(name);
        if (iter == profile_printers.end())
            return std::string();

        ProfilePrinter* printer = iter->second;
        return printer->vendor;
    }
    else
    {
        std::string filamentVentor = getFilamentVendor(preset);
        return filamentVentor;
    }
}

void ParamsPanel::getDatas(std::vector<wxString>& systemPrintList, std::vector<wxString>& userPrintList, std::vector<wxString>& projectList,
    std::unordered_set<wxString>&  printList, std::unordered_set<wxString>& ventorList)
{
    //PresetCollection* presetCollection = &(wxGetApp().preset_bundle->filaments);
    //const std::deque<Preset>& presets1 = presetCollection->get_presets();
    //std::map<std::string, std::vector<Preset const*>> filament_list = wxGetApp().preset_bundle->filaments.get_filament_presets();
    //size_t pSize = presets1.size();
    std::string res = wxGetApp().app_config->get("role_type");
    if (res == "0")
    {
        m_ps = PS_BASE;
    }
    std::deque<Preset> presets;
    size_t count;
    size_t idx_selected;
    if (m_ws == WS_PRINTER)
    {
        count = wxGetApp().preset_bundle->printers.num_default_presets();
        idx_selected = wxGetApp().preset_bundle->printers.get_selected_idx();
        presets = wxGetApp().preset_bundle->printers.get_presets();
    }
    else if(m_ws == WS_FILAMENT)
    {
        count = wxGetApp().preset_bundle->filaments.num_default_presets();
        idx_selected = wxGetApp().preset_bundle->filaments.get_selected_idx();
        presets = wxGetApp().preset_bundle->filaments.get_presets();
    }
    PresetBundle* preset_bundle = wxGetApp().preset_bundle;
    for (size_t i = presets.front().is_visible ? 0 : count; i < presets.size(); ++i)
    {
        Preset preset = presets[i];
        if (!preset.is_visible || (!preset.is_compatible && i != idx_selected) || !preset.m_is_user_presets)
            continue;
        //Preset preset = presets[i];

        wxString ventor = getVendor(preset);
        wxString presetType = getPresetType(preset);
        wxString itemName = from_u8(preset.label(true));


        if (preset.is_default || preset.is_system)
        {
            if ((m_ps == PS_SYSTEM || m_ps == PS_BASE))
            {
                if (ventor != "")
                    ventorList.insert(ventor);

                wxString firstValue;
                for (const auto& data : ventorList)
                {
                    firstValue = data;
                    break;
                }
                
                m_curVentor = m_curVentor == "" ? firstValue : m_curVentor;

                if (presetType != "" && ventor == m_curVentor || m_curVentor == _L("ALL").ToStdString())
                    printList.insert(presetType);

                wxString firstType;
                for (const auto& data : printList)
                {
                    firstType = data;
                    break;
                }
                m_printerType = m_printerType == "" ? firstType : m_printerType;

                if (ventor == m_curVentor || m_curVentor == _L("ALL").ToStdString() || res == "0")
                {
                    if (presetType == m_printerType || m_printerType == _L("ALL").ToStdString())
                    {
                        systemPrintList.push_back(itemName);
                    }
                }
            }
        }
        else {
            if (m_ps == PS_USER || m_ps == PS_BASE)
            {
                if (ventor != "")
                    ventorList.insert(ventor);

                wxString firstValue;
                for (const auto& data : ventorList)
                {
                    firstValue = data;
                    break;
                }

                m_curVentor = m_curVentor == "" ? firstValue : m_curVentor;

                if (presetType != "" && ventor == m_curVentor || m_curVentor == _L("ALL").ToStdString())
                    printList.insert(presetType);
                
                std::string firstType;
                for (const auto& data : printList)
                {
                    firstValue = data;
                    break;
                }

                m_printerType = m_printerType == "" ? firstValue : m_printerType;

                if (ventor == m_curVentor || m_curVentor == _L("ALL").ToStdString() || res == "0")
                {
                    if (presetType == m_printerType || m_printerType == _L("ALL").ToStdString())
                    {
                        if (preset.is_project_embedded)
                        {
                            projectList.push_back(itemName);
                        }
                        else {
                            userPrintList.push_back(itemName);
                        }
                       
                    }
                }
                /*if (((presetType == m_printerType && ventor == m_curVentor )|| res == "0" || m_printerType == _L("ALL").ToStdString()))
                    userPrintList.push_back(itemName);*/
            }
        }
    }
}

void ParamsPanel::updateVentors(const std::unordered_set<wxString>& ventors)
{
    if (m_ws == WS_PRINTER)
    {
        TabPrinter* tp = dynamic_cast<TabPrinter*>(m_tab_printer);
        if (tp)
        {
            tp->updateVentorList(ventors, _L("ALL").ToStdString());
        }
    }
    else 
    {
        TabFilament* tf = dynamic_cast<TabFilament*>(m_tab_filament);
        if (tf)
        {
            tf->updateVentorList(ventors, _L("ALL").ToStdString());
        }
    }
}

void ParamsPanel::updatePrinters(const std::unordered_set<wxString>& printers)
{
    if (m_ws == WS_PRINTER)
    {
        TabPrinter* tp = dynamic_cast<TabPrinter*>(m_tab_printer);
        if (tp)
        {
            tp->updatePrintList(printers, _L("ALL").ToStdString());
        }
    }
    else
    {
        TabFilament* tf = dynamic_cast<TabFilament*>(m_tab_filament);
        if (tf)
        {
            tf->updatePrintList(printers, _L("ALL").ToStdString());
        }
    }
}

void ParamsPanel::updatePresetsList(const std::vector<wxString>& systemList, const std::vector<wxString>& userList,
    const std::vector<wxString>& projectList)
{
    m_preset_listBox->DeleteAllItems();
    m_preset_listBox->SetIndent(0);

    wxDataViewItemAttr attr;
    attr.SetBackgroundColour(wxColor(255, 0, 0));

    if (projectList.size()) {
        wxDataViewItem child1 = m_preset_listBox->AppendContainer(wxDataViewItem(), _L("Project presets"));
        for (auto& projectName : projectList) {
            m_preset_listBox->AppendItem(child1, projectName);
        }
        m_preset_listBox->Expand(child1);
    }

    if (userList.size()) {
        wxDataViewItem child1 = m_preset_listBox->AppendContainer(wxDataViewItem(), _L("User presets"));
        for (auto& printName : userList) {
            m_preset_listBox->AppendItem(child1, printName);
        }
        m_preset_listBox->Expand(child1);
    }    

    if (systemList.size()) {
        wxDataViewItem child2 = m_preset_listBox->AppendContainer(wxDataViewItem(), _L("System presets"));
        for (auto& printName : systemList) {
            m_preset_listBox->AppendItem(child2, printName);
        }
        m_preset_listBox->Expand(child2);
    }
    
    m_preset_listBox->Bind(wxEVT_DATAVIEW_ITEM_EDITING_STARTED, [=](wxDataViewEvent& e) {
        wxDataViewColumn* c2 = m_preset_listBox->GetCurrentColumn();
        if (c2) {
            wxDataViewRenderer* renderer = c2->GetRenderer();
            renderer->SetMode(wxDATAVIEW_CELL_INERT);
            renderer->CancelEditing();
        }
        });
    wxFont treeFont = m_preset_listBox->GetFont();
    treeFont.SetPointSize(10);
    m_preset_listBox->SetFont(treeFont);
    SelectRowByString(m_curPreset);
}

void ParamsPanel::setCurVentor(const std::string& curVentor)
{
    if (m_curVentor == curVentor)
        return;
    m_curVentor = curVentor;
    m_printerType = _L("ALL").ToStdString();
    filteredData(m_curVentor, m_printerType, 1);
}

void ParamsPanel::setCurType(const std::string& curType)
{
    if (m_printerType == curType)
        return;
    m_printerType = curType;
    filteredData(m_curVentor, m_printerType, 2);
}

std::string ParamsPanel::getPresetType(Preset preset)
{
    std::string presetType;
    if (m_ws == WS_PRINTER)
    {
        presetType = preset.config.option<ConfigOptionString>("printer_model", true)->value;
    }
    else if(m_ws == WS_FILAMENT)
    {
        PresetBundle* preset_bundle = wxGetApp().preset_bundle;
        Preset* inherit_preset = nullptr;
        auto inherit = dynamic_cast<ConfigOptionString*>(preset.config.option(BBL_JSON_KEY_INHERITS, false));
        if (inherit && !inherit->value.empty()) {
            std::string inherits_value = inherit->value;
            inherit_preset = preset_bundle->filaments.find_preset(inherits_value, false, true);
        }

        ConfigOptionStrings* filament_types;
        if (!inherit_preset) {
            filament_types = dynamic_cast<ConfigOptionStrings*>(preset.config.option("filament_type"));
        }
        else {
            filament_types = dynamic_cast<ConfigOptionStrings*>(inherit_preset->config.option("filament_type"));
        }

        //if (filament_types && filament_types->values.empty()) continue;
        if (filament_types && filament_types->size() > 0)
            presetType = filament_types->values[0];
        else
            presetType = std::string();
    }
    return presetType;
}

void ParamsPanel::SelectRowByString(const wxString& text)
{
    // 从根节点开始遍历
    wxDataViewItem rootItem;
    wxDataViewItemArray children;
    m_preset_listBox->GetStore()->GetChildren(rootItem, children);

    for (auto& item : children)
    {
        if (SelectRowRecursive(item, text))
        {
            break;
        }
    }
}

bool ParamsPanel::SelectRowRecursive(const wxDataViewItem& item, const wxString& text)
{
    wxVariant value;
    wxString itemText = m_preset_listBox->GetItemText(item);
    if (itemText == text)
    {
        m_preset_listBox->Select(item);
        return true;
    }

    // 遍历子节点
    wxDataViewItemArray children;
    m_preset_listBox->GetStore()->GetChildren(item, children);

    for (auto& child : children)
    {
        if (SelectRowRecursive(child, text))
        {
            return true;
        }
    }

    return false;
}

std::vector<std::string> ParamsPanel::filterVentor()
{
    std::set<std::string> ventors;
    std::set<std::string> sysKeys;
    for (const auto& pair : m_systemDatas) {
        sysKeys.insert(pair.first);
    }

    std::set<std::string> userKeys;
    for (const auto& pair : m_userDatas) {
        userKeys.insert(pair.first);
    }

    if (m_ps == PS_SYSTEM)
    {
        ventors = sysKeys;
    }
    else if (m_ps == PS_USER)
    {
        ventors = userKeys;
    }
    else if (m_ps == PS_BASE)
    {
        ventors.insert(sysKeys.begin(), sysKeys.end());
        ventors.insert(userKeys.begin(), userKeys.end());
    }

    std::vector<std::string> res;
    for (const auto& pair : ventors) {
        res.push_back(pair);
    }

    return res;
}

std::vector<std::string> ParamsPanel::filterPresetType(const std::string& vector)
{
    std::vector<std::multimap<std::string, Preset>> result;
    auto range = m_userDatas.equal_range(vector);
    for (auto it = range.first; it != range.second; ++it) {
        result.push_back(it->second);
    }
    return std::vector<std::string>();
}

void ParamsPanel::OnSelectName(const wxString& name)
{
    if (!m_tabbar_ctrl)
        return;

    m_tabbar_ctrl->OnSelectName(name);
}

void ParamsPanel::OnSelectPrinterWildVendorModel()
{
    m_tabbar_printer->OnSelectWildVendorModel();
}
void ParamsPanel::OnSelectPrinterWildVendor()
{
    m_tabbar_printer->OnSelectWildVendor();
}
void ParamsPanel::OnSelectPrinterVendor(const wxString& vendor)
{
    m_tabbar_printer->OnSelectVendor(vendor);
}
void ParamsPanel::OnSelectPrinterWildModel(const wxString& vendor)
{
    m_tabbar_printer->OnSelectWildModel(vendor);
}
void ParamsPanel::OnSelectPrinterVendorModel(const wxString& vendor, const wxString& model)
{
    m_tabbar_printer->OnSelectVendorModel(vendor, model);
}
void ParamsPanel::OnSelectPrinterName(const wxString& name)
{
    m_tabbar_printer->OnSelectName(name);
}

void ParamsPanel::layoutPrinterAndFilament()
{
    bool is_dark = wxGetApp().dark_mode();
    wxBoxSizer* buttons_sizer = new wxBoxSizer(wxHORIZONTAL);

    int em = em_unit(this);
    StateColor text_color = StateColor(std::pair{ is_dark ? wxColour(254, 254, 254) : wxColour(0,0,0), (int)StateColor::Normal });
    StateColor bg_color = StateColor(std::pair{ wxColour(23, 204, 95), (int)StateColor::Hovered },
        std::pair{ is_dark ? wxColour(110, 110, 115) : wxColour(214, 214, 220), (int)StateColor::Normal });


    //main_sizer->Add(sizer, 1, wxEXPAND | wxALL, FromDIP(10));
    //main_sizer->Add(tmp_sizer, 1, wxEXPAND | wxALL, FromDIP(10));
    //
    m_color_border_box->SetSizer(m_tmp_sizer);
    //m_color_border_box->SetBackgroundColour(wxColour(0, 255, 0));
  
    if (m_mode_region)
        m_mode_region->Bind(wxEVT_TOGGLEBUTTON, &ParamsPanel::OnToggled, this);
    if (m_mode_view)
        m_mode_view->Bind(wxEVT_TOGGLEBUTTON, &ParamsPanel::OnToggled, this);
    Bind(wxEVT_TOGGLEBUTTON, &ParamsPanel::OnToggled, this); // For Tab's mode switch
    //Bind(wxEVT_BUTTON, [this](wxCommandEvent &) { wxGetApp().plater()->search(false); }, wxID_FIND);
    //m_export_to_file->Bind(wxEVT_BUTTON, [this](wxCommandEvent &) { wxGetApp().mainframe->export_config(); });
    //m_import_from_file->Bind(wxEVT_BUTTON, [this](wxCommandEvent &) { wxGetApp().mainframe->load_config_file(); });

   /* m_h_tabbar_printer = new HTabbarPrinter(this, Preset::Type::TYPE_PRINTER, wxGetApp().preset_bundle);
    m_v_tabbar_printer = new VTabbarPrinter(this, Preset::Type::TYPE_PRINTER, wxGetApp().preset_bundle);*/

    //m_tabbar_printer = new TabbarCtrlPrinter(this, m_h_tabbar_printer, m_v_tabbar_printer, wxGetApp().preset_bundle);
    //m_tabbar_filament = new TabbarCtrlFilament(this, m_h_tabbar_printer, m_v_tabbar_printer, wxGetApp().preset_bundle);

    StateColor btn_bg_white(std::pair<wxColour, int>(wxColour(206, 206, 206), StateColor::Pressed),
        std::pair<wxColour, int>(wxColour(238, 238, 238), StateColor::Hovered),
        std::pair<wxColour, int>(*wxWHITE, StateColor::Normal));
    {
        m_button_save = new Button(this, _L("Save"));
        m_button_save->SetBackgroundColor(btn_bg_white);
        m_button_save->SetBorderColor(wxColour(38, 46, 48));
        //m_button_save->SetTextColor(wxColour(0xFFFFFE));
        m_button_save->SetFont(Label::Body_12);
        m_button_save->SetSize(wxSize(FromDIP(58), FromDIP(24)));
        m_button_save->SetMinSize(wxSize(FromDIP(58), FromDIP(24)));
        m_button_save->SetCornerRadius(FromDIP(12));
    }
    {
        //m_button_save_as = new Button(this, _L("Save as"));
        //m_button_save_as->SetBackgroundColor(btn_bg_white);
        //m_button_save_as->SetBorderColor(wxColour(38, 46, 48));
        ////m_button_save_as->SetTextColor(wxColour(0xFFFFFE));
        //m_button_save_as->SetFont(Label::Body_12);
        //m_button_save_as->SetSize(wxSize(FromDIP(58), FromDIP(24)));
        //m_button_save_as->SetMinSize(wxSize(FromDIP(58), FromDIP(24)));
        //m_button_save_as->SetCornerRadius(FromDIP(12));
    }
    {
        m_button_delete = new Button(this, _L("Delete"));
        m_button_delete->SetBackgroundColor(btn_bg_white);
        m_button_delete->SetBorderColor(wxColour(38, 46, 48));
        //m_button_delete->SetTextColor(wxColour(0xFFFFFE));
        m_button_delete->SetFont(Label::Body_12);
        m_button_delete->SetSize(wxSize(FromDIP(58), FromDIP(24)));
        m_button_delete->SetMinSize(wxSize(FromDIP(58), FromDIP(24)));
        m_button_delete->SetCornerRadius(FromDIP(12));
    }
    {
        m_button_reset = new Button(this, _L("Reset"));
        m_button_reset->SetBackgroundColor(btn_bg_white);
        m_button_reset->SetBorderColor(wxColour(38, 46, 48));
        //m_button_reset->SetTextColor(wxColour(0xFFFFFE));
        m_button_reset->SetFont(Label::Body_12);
        m_button_reset->SetSize(wxSize(FromDIP(58), FromDIP(24)));
        m_button_reset->SetMinSize(wxSize(FromDIP(58), FromDIP(24)));
        m_button_reset->SetCornerRadius(FromDIP(12));
    }

    m_button_save->Bind(wxEVT_BUTTON, ([this](wxCommandEvent e)
        {
            if (!m_current_tab)
                return;

            Tab* cur_tab = dynamic_cast<Tab*> (m_current_tab);
            cur_tab->save_preset();
            OnPanelShowInit();
        }));

    m_button_delete->Bind(wxEVT_BUTTON, ([this](wxCommandEvent e)
        {
            if (!m_current_tab)
                return;

            Tab* cur_tab = dynamic_cast<Tab*> (m_current_tab);
            cur_tab->delete_preset();
            OnPanelShowInit();
        }));

    //m_button_save_as->Bind(wxEVT_BUTTON, ([this](wxCommandEvent e)
    //    {
    //        if (!m_current_tab)
    //            return;

    //        Tab* cur_tab = dynamic_cast<Tab*> (m_current_tab);
    //        cur_tab->save_preset();
    //    }));

    m_button_reset->Bind(wxEVT_BUTTON, ([this](wxCommandEvent e)
        {
            if (!m_current_tab)
                return;

            Tab* cur_tab = dynamic_cast<Tab*> (m_current_tab);
            cur_tab->on_roll_back_value();
        }));
}

void ParamsPanel::layoutOther()
{
    bool is_dark = wxGetApp().dark_mode();
    m_color_border_box->SetSizer(m_tmp_sizer);

    if (m_mode_region)
        m_mode_region->Bind(wxEVT_TOGGLEBUTTON, &ParamsPanel::OnToggled, this);
    if (m_mode_view)
        m_mode_view->Bind(wxEVT_TOGGLEBUTTON, &ParamsPanel::OnToggled, this);
    Bind(wxEVT_TOGGLEBUTTON, &ParamsPanel::OnToggled, this); // For Tab's mode switch
    //Bind(wxEVT_BUTTON, [this](wxCommandEvent &) { wxGetApp().plater()->search(false); }, wxID_FIND);
    //m_export_to_file->Bind(wxEVT_BUTTON, [this](wxCommandEvent &) { wxGetApp().mainframe->export_config(); });
    //m_import_from_file->Bind(wxEVT_BUTTON, [this](wxCommandEvent &) { wxGetApp().mainframe->load_config_file(); });

    m_h_tabbar_printer = new HTabbarPrinter(this, Preset::Type::TYPE_PRINTER, wxGetApp().preset_bundle);
    m_v_tabbar_printer = new VTabbarPrinter(this, Preset::Type::TYPE_PRINTER, wxGetApp().preset_bundle);

    m_tabbar_printer = new TabbarCtrlPrinter(this, m_h_tabbar_printer, m_v_tabbar_printer, wxGetApp().preset_bundle);
    m_tabbar_filament = new TabbarCtrlFilament(this, m_h_tabbar_printer, m_v_tabbar_printer, wxGetApp().preset_bundle);

    StateColor btn_bg_white(std::pair<wxColour, int>(wxColour(206, 206, 206), StateColor::Pressed),
        std::pair<wxColour, int>(wxColour(238, 238, 238), StateColor::Hovered),
        std::pair<wxColour, int>(*wxWHITE, StateColor::Normal));
    {
        m_button_save = new Button(this, _L("Save"));
        m_button_save->SetBackgroundColor(btn_bg_white);
        m_button_save->SetBorderColor(wxColour(38, 46, 48));
        //m_button_save->SetTextColor(wxColour(0xFFFFFE));
        m_button_save->SetFont(Label::Body_12);
        m_button_save->SetSize(wxSize(FromDIP(58), FromDIP(24)));
        m_button_save->SetMinSize(wxSize(FromDIP(58), FromDIP(24)));
        m_button_save->SetCornerRadius(FromDIP(12));
    }
    {
        //m_button_save_as = new Button(this, _L("Save as"));
        //m_button_save_as->SetBackgroundColor(btn_bg_white);
        //m_button_save_as->SetBorderColor(wxColour(38, 46, 48));
        ////m_button_save_as->SetTextColor(wxColour(0xFFFFFE));
        //m_button_save_as->SetFont(Label::Body_12);
        //m_button_save_as->SetSize(wxSize(FromDIP(58), FromDIP(24)));
        //m_button_save_as->SetMinSize(wxSize(FromDIP(58), FromDIP(24)));
        //m_button_save_as->SetCornerRadius(FromDIP(12));
    }
    {
        m_button_delete = new Button(this, _L("Delete"));
        m_button_delete->SetBackgroundColor(btn_bg_white);
        m_button_delete->SetBorderColor(wxColour(38, 46, 48));
        //m_button_delete->SetTextColor(wxColour(0xFFFFFE));
        m_button_delete->SetFont(Label::Body_12);
        m_button_delete->SetSize(wxSize(FromDIP(58), FromDIP(24)));
        m_button_delete->SetMinSize(wxSize(FromDIP(58), FromDIP(24)));
        m_button_delete->SetCornerRadius(FromDIP(12));
    }
    {
        m_button_reset = new Button(this, _L("Reset"));
        m_button_reset->SetBackgroundColor(btn_bg_white);
        m_button_reset->SetBorderColor(wxColour(38, 46, 48));
        //m_button_reset->SetTextColor(wxColour(0xFFFFFE));
        m_button_reset->SetFont(Label::Body_12);
        m_button_reset->SetSize(wxSize(FromDIP(58), FromDIP(24)));
        m_button_reset->SetMinSize(wxSize(FromDIP(58), FromDIP(24)));
        m_button_reset->SetCornerRadius(FromDIP(12));
    }

    m_button_save->Bind(wxEVT_BUTTON, ([this](wxCommandEvent e)
        {
            if (!m_current_tab)
                return;

            Tab* cur_tab = dynamic_cast<Tab*> (m_current_tab);
            cur_tab->save_preset();
            OnPanelShowInit();
        }));

    m_button_delete->Bind(wxEVT_BUTTON, ([this](wxCommandEvent e)
        {
            if (!m_current_tab)
                return;

            Tab* cur_tab = dynamic_cast<Tab*> (m_current_tab);
            cur_tab->delete_preset();
            OnPanelShowInit();
        }));

    //m_button_save_as->Bind(wxEVT_BUTTON, ([this](wxCommandEvent e)
    //    {
    //        if (!m_current_tab)
    //            return;

    //        Tab* cur_tab = dynamic_cast<Tab*> (m_current_tab);
    //        cur_tab->save_preset();
    //    }));

    m_button_reset->Bind(wxEVT_BUTTON, ([this](wxCommandEvent e)
        {
            if (!m_current_tab)
                return;

            Tab* cur_tab = dynamic_cast<Tab*> (m_current_tab);
            cur_tab->on_roll_back_value();
        }));
}

ProcessParamsPanel::ProcessParamsPanel(wxWindow* parent, wxWindowID id, const wxPoint& pos,
    const wxSize& size, long style, const wxString& type)
    :ParamsPanel(parent, id, pos, size, style, type)
{

}

ProcessParamsPanel::~ProcessParamsPanel()
{

}

void ProcessParamsPanel::create_layout()
{
    layoutOther();
    create_layout_process();
}

} // GUI
} // Slic3r
