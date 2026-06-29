#include "libslic3r/libslic3r.h"
#include "KBShortcutsDialog.hpp"
#include "I18N.hpp"
#include "libslic3r/Utils.hpp"
#include "GUI.hpp"
#include "Notebook.hpp"
#include <wx/scrolwin.h>
#include <wx/display.h>
#include "GUI_App.hpp"
#include "wxExtensions.hpp"
#include "MainFrame.hpp"
#include <wx/notebook.h>
#include "libslic3r/common_header/common_header.h"
#include <wx/glcanvas.h>
#include "GLCanvas3D.hpp"
namespace Slic3r {
namespace GUI {

wxDEFINE_EVENT(EVT_PREFERENCES_SELECT_TAB, wxCommandEvent);

SelectableCard::SelectableCard(wxWindow* parent, wxString title)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxBORDER_NONE), m_title(std::move(title))
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    Bind(wxEVT_PAINT, &SelectableCard::OnPaint, this);
    Bind(wxEVT_ERASE_BACKGROUND, [](wxEraseEvent&) {}); 
}

void SelectableCard::OnPaint(wxPaintEvent&)
{
    wxAutoBufferedPaintDC dc(this);
    const wxRect          rc = GetClientRect();

    wxColour bg = GetBackgroundColour();
    if (!bg.IsOk() && GetParent())
        bg = GetParent()->GetBackgroundColour();
    if (!bg.IsOk())
        bg = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.SetBrush(wxBrush(bg));
    dc.DrawRectangle(rc);

    // 颜色 & 尺寸
    const wxColour colBorderSel(21, 192, 89);  // 选中边
    const wxColour colBorderNor(150, 150, 152); // 未选边
    const wxColour colBorderHov(190, 190, 190); // 悬停
    const wxColour colTitle(145, 149, 153); //标题字体颜色
    const wxColour colDivider(150, 150, 152);//分割线

    const int border_w = m_selected ? FromDIP(2) : FromDIP(1);
    const int pad      = FromDIP(8);
    const int header_h = HeaderHeight();

    // 外框
    wxRect rOuter = rc;
    rOuter.Deflate(FromDIP(2));
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.SetPen(wxPen(m_selected ? colBorderSel : (m_hover ? colBorderHov : colBorderNor), border_w));
    dc.DrawRoundedRectangle(rOuter,0);

    // 内容区域
    wxRect rInner = rOuter;
    rInner.Deflate(FromDIP(3));

    // 顶部标题条
    wxRect rHeader = rInner;
    rHeader.height = header_h;

    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawRectangle(rHeader);

    // 标题文字
    dc.SetTextForeground(colTitle);
    wxFont f = GetFont();
    f.SetWeight(wxFONTWEIGHT_SEMIBOLD);
    dc.SetFont(f);
    const int tx = rHeader.GetX() + pad;
    const int ty = rHeader.GetY() + (header_h - dc.GetCharHeight()) / 2;
    dc.DrawText(m_title, tx, ty);

    dc.SetPen(wxPen(colDivider, FromDIP(1)));
    const int line_y = rHeader.GetBottom();
    dc.DrawLine(rHeader.GetLeft(), line_y, rInner.GetRight(), line_y);

    if (m_selected) {
        const int badge = FromDIP(22);
        const int bx    = rInner.GetRight() - badge;
        const int by    = rHeader.GetY();

        dc.SetBrush(wxBrush(colBorderSel));
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.DrawRectangle(wxRect(bx, by, badge, badge));

         wxPen pen(*wxWHITE, FromDIP(2), wxPENSTYLE_SOLID);
        pen.SetCap(wxCAP_ROUND);
        pen.SetJoin(wxJOIN_ROUND);
        dc.SetPen(pen);

        const int ip = FromDIP(5); // 勾的内边距
        wxPoint   p1(bx + ip, by + badge / 2);
        wxPoint   p2(bx + badge / 2 - 1, by + badge - ip);
        wxPoint   p3(bx + badge - ip, by + ip);
        dc.DrawLine(p1, p2);
        dc.DrawLine(p2, p3);
    }
}

KBShortcutsDialog::KBShortcutsDialog()
    : DPIDialog(static_cast<wxWindow*>(wxGetApp().mainframe), wxID_ANY,_L("Keyboard Shortcuts"),
    wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE)
{
    // fonts
    const wxFont& font = wxGetApp().normal_font();
    const wxFont& bold_font = wxGetApp().bold_font();
    SetFont(font);

    std::string icon_path = (boost::format("%1%/images/%2%.ico") % resources_dir() % Slic3r::CxBuildInfo::getIconName()).str();
    SetIcon(wxIcon(encode_path(icon_path.c_str()), wxBITMAP_TYPE_ICO));

    this->SetSizeHints(wxDefaultSize, wxDefaultSize);
    this->SetBackgroundColour(wxColour(255, 255, 255));

    wxBoxSizer *m_sizer_top = new wxBoxSizer(wxVERTICAL);

    auto m_top_line = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 1), wxTAB_TRAVERSAL);
    m_top_line->SetBackgroundColour(wxColour(166, 169, 170));

    m_sizer_top->Add(m_top_line, 0, wxEXPAND, 0);
    m_sizer_body = new wxBoxSizer(wxHORIZONTAL);

    m_panel_selects = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
    m_panel_selects->SetBackgroundColour(wxColour(248, 248, 248));
    wxBoxSizer *m_sizer_left = new wxBoxSizer(wxVERTICAL);

    m_sizer_left->Add(0, 0, 0, wxEXPAND | wxTOP, FromDIP(20));

    m_sizer_left->Add(create_button(0, _L("Global")), 0, wxEXPAND, 0);
    m_sizer_left->Add(create_button(1, _L("Prepare")), 0, wxEXPAND, 0);
    m_sizer_left->Add(create_button(2, _L("Toolbar")), 0, wxEXPAND, 0);
    m_sizer_left->Add(create_button(3, _L("Objects list")), 0, wxEXPAND, 0);
    m_sizer_left->Add(create_button(4, _L("Preview")), 0, wxEXPAND, 0);

    m_panel_selects->SetSizer(m_sizer_left);
    m_panel_selects->Layout();
    m_sizer_left->Fit(m_panel_selects);
    m_sizer_body->Add(m_panel_selects, 0, wxEXPAND, 0);

    m_sizer_right = new wxBoxSizer(wxHORIZONTAL);

    m_sizer_right->Add(0, 0, 0, wxEXPAND | wxLEFT,  FromDIP(12));

    m_simplebook = new wxSimplebook(this, wxID_ANY, wxDefaultPosition, wxSize(FromDIP(870), FromDIP(500)), 0);

    m_sizer_right->Add(m_simplebook, 1, wxEXPAND, 0);
    m_sizer_body->Add(m_sizer_right, 1, wxEXPAND, 0);
    m_sizer_top->Add(m_sizer_body, 1, wxEXPAND, 0);

    fill_shortcuts();
    for (size_t i = 0; i < m_full_shortcuts.size(); ++i) {
        wxPanel *page = create_page(m_simplebook, m_full_shortcuts[i], font, bold_font);
        m_pages.push_back(page);
        m_simplebook->AddPage(page, m_full_shortcuts[i].first.first, i == 0);
    }

    Bind(EVT_PREFERENCES_SELECT_TAB, &KBShortcutsDialog::OnSelectTabel, this);

    SetSizer(m_sizer_top);
    Layout();
    Fit();
    CenterOnParent();

    // select first
    auto event = wxCommandEvent(EVT_PREFERENCES_SELECT_TAB);
    event.SetInt(0);
    event.SetEventObject(this);
    wxPostEvent(this, event);
    wxGetApp().UpdateDlgDarkUI(this);
}

static wxSizer* make_kv(wxWindow* parent, const wxString& key, const wxString& desc)
{
    auto* row = new wxBoxSizer(wxHORIZONTAL);
    auto* k   = new wxStaticText(parent, wxID_ANY, key);
    auto* sep = new wxStaticText(parent, wxID_ANY, ": ");
    auto* d   = new wxStaticText(parent, wxID_ANY, desc);
    // Ensure static texts have transparent background across platforms.
    // Otherwise, on Linux the parent panel's custom painting may overlap
    // and visually clip the glyphs.
    k->SetBackgroundStyle(wxBG_STYLE_TRANSPARENT);
    sep->SetBackgroundStyle(wxBG_STYLE_TRANSPARENT);
    d->SetBackgroundStyle(wxBG_STYLE_TRANSPARENT);
    k->SetFont(k->GetFont().Bold());
    // Prevent wrapping in the key/desc rows inside mouse scheme cards.
    k->Wrap(-1);
    d->Wrap(10000);
    // Reserve enough width for the key label to avoid squeezing the last
    // character to a next line on GTK.
    k->SetMinSize(wxSize(k->GetBestSize().GetWidth(), -1));
    row->Add(k, 0, wxALIGN_CENTER_VERTICAL);
    row->Add(sep, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, parent->FromDIP(20));
    row->Add(d, 0, wxALIGN_CENTER_VERTICAL);
    return row;
}

void SelectableCard::SetSelected(bool sel)
{
    if (m_selected != sel) 
    {
        m_selected = sel;
        Refresh();
    }
}
bool SelectableCard::IsSelected() const 
{ 
    return m_selected; 
}

void KBShortcutsDialog::create_mouse_scheme_cards(wxWindow* parent)
{
    auto* h = parent->GetSizer();
    if (!h) {
        h = new wxBoxSizer(wxHORIZONTAL);
        parent->SetSizer(h);
    }

    auto add_card = [&](SelectableCard*& card, const wxString& title, std::initializer_list<std::pair<wxString, wxString>> rows) {
        card = new SelectableCard(parent, title);
        card->SetBackgroundColour(parent->GetBackgroundColour());

        auto* v = new wxBoxSizer(wxVERTICAL);
        v->AddSpacer(card->HeaderHeight() + FromDIP(12));

        // 使用 FlexGridSizer 构建两列（键、描述），让第二列自适应
        wxFlexGridSizer* grid = new wxFlexGridSizer((int)rows.size(), 2, FromDIP(8), FromDIP(20));
        grid->AddGrowableCol(1, 1);
        grid->SetFlexibleDirection(wxBOTH);
        grid->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_SPECIFIED);

        // 计算键列的最大文本宽度，确保不折行并与下方列表一致
        int max_key_width = 0;
        {
            wxClientDC dc(card);
            wxFont bold = card->GetFont();
            bold.SetWeight(wxFONTWEIGHT_BOLD);
            dc.SetFont(bold);
            for (const auto& kv : rows) {
                wxSize ext;
                dc.GetTextExtent(kv.first, &ext.x, &ext.y);
                max_key_width = std::max(max_key_width, ext.x);
            }
            max_key_width += FromDIP(4);
        }

        for (const auto& kv : rows) {
            auto* key = new wxStaticText(card, wxID_ANY, kv.first);
            key->SetBackgroundStyle(wxBG_STYLE_TRANSPARENT);
            key->SetFont(key->GetFont().Bold());
            key->Wrap(-1); // 键列不换行
            key->SetMinSize(wxSize(max_key_width, -1));
            grid->Add(key, 0, wxALIGN_CENTER_VERTICAL);

            auto* desc = new wxStaticText(card, wxID_ANY, kv.second);
            desc->SetBackgroundStyle(wxBG_STYLE_TRANSPARENT);
            desc->Wrap(10000); // 描述列默认不换行，空间不足时由 sizer 处理
            grid->Add(desc, 0, wxALIGN_CENTER_VERTICAL | wxEXPAND);
        }

        v->Add(grid, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(12));

        card->SetSizer(v);
        // 取消固定最小宽度，让左右两卡片按父容器宽度对半分
        card->SetMinSize(wxSize(wxDefaultCoord, wxDefaultCoord));
        card->SetCursor(wxCursor(wxCURSOR_HAND));
        return card;
    };

    #if __APPLE__
    add_card(reinterpret_cast<SelectableCard*&>(m_card_scheme_a), _L("Mouse shortcut scheme 1"),
             {
                 {_L("Click"), _L("Rotate View")},
                 {_L("Control+Click"), _L("Pan View")},
                 {_L("Shift+mouse"), _L("Select objects by rectangle")},
             });

    add_card(reinterpret_cast<SelectableCard*&>(m_card_scheme_b), _L("Mouse shortcut scheme 2"),
             {
                 {_L("Click"), _L("Rotate View")},
                 {_L("Right mouse button"), _L("Pan View")},
                 {_L("Shift+mouse"), _L("Select objects by rectangle")},
             });

    #else
    add_card(reinterpret_cast<SelectableCard*&>(m_card_scheme_a), _L("Mouse shortcut scheme 1"),
             {
                 {_L("Left mouse button"), _L("Select objects by rectangle")},
                 {_L("Right mouse button"), _L("Rotate View")},
                 {_L("Shift+Left mouse button"), _L("Pan View")},
             });

    add_card(reinterpret_cast<SelectableCard*&>(m_card_scheme_b), _L("Mouse shortcut scheme 2"),
             {
                 {_L("Left mouse button"), _L("Rotate View")},
                 {_L("Right mouse button"), _L("Pan View")},
                 {_L("Shift+Left mouse button"), _L("Select objects by rectangle")},
             });
    #endif
    h->Add(m_card_scheme_a, 1, wxEXPAND | wxALL, FromDIP(6));
    h->Add(m_card_scheme_b, 1, wxEXPAND | wxALL, FromDIP(6));

    m_card_scheme_a->Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent&) { select_scheme(true); });
    m_card_scheme_b->Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent&) { select_scheme(false); });

    static_cast<SelectableCard*>(m_card_scheme_a)->SetSelected(true);
    static_cast<SelectableCard*>(m_card_scheme_b)->SetSelected(false);
    m_is_a_selected = true;

    auto make_texts_transparent = [](wxWindow* panel) {
        // Make all static texts transparent so custom painting of SelectableCard
        // doesn't cover or clip text on non-Windows platforms.
        for (auto* child : panel->GetChildren())
            if (auto* st = wxDynamicCast(child, wxStaticText)) {
                st->SetBackgroundStyle(wxBG_STYLE_TRANSPARENT);
                st->SetBackgroundColour(wxNullColour);
            }
    };
    make_texts_transparent(m_card_scheme_a);
    make_texts_transparent(m_card_scheme_b);

    auto bind_click_recursive = [&](wxWindow* w, bool choose_a, auto&& self) -> void {
        w->Bind(wxEVT_LEFT_DOWN, [this, choose_a](wxMouseEvent&) { select_scheme(choose_a); });

        for (wxWindow* child : w->GetChildren())
            self(child, choose_a, self);
    };
    bind_click_recursive(m_card_scheme_a, true, bind_click_recursive);
    bind_click_recursive(m_card_scheme_b, false, bind_click_recursive);

    auto bind_hover_guard = [&](wxWindow* card_w) {
        auto* card = static_cast<SelectableCard*>(card_w);

        card_w->Bind(wxEVT_ENTER_WINDOW, [card](wxMouseEvent& e) {
            card->SetHover(true);
            e.Skip();
        });

        card_w->Bind(wxEVT_LEAVE_WINDOW, [card](wxMouseEvent& e) {
            card->CallAfter([card] {
                const wxPoint mouse_sp = wxGetMousePosition();
                const wxRect  card_scr = card->GetScreenRect();
                const bool    inside   = card_scr.Contains(mouse_sp);
                card->SetHover(inside);
            });
            e.Skip();
        });

        card_w->Bind(wxEVT_MOTION, [card](wxMouseEvent& e) {
            card->SetHover(true);
            e.Skip();
        });
    };

    bind_hover_guard(m_card_scheme_a);
    bind_hover_guard(m_card_scheme_b);
}


void KBShortcutsDialog::select_scheme(bool choose_a)
{
    m_is_a_selected = choose_a;
    static_cast<SelectableCard*>(m_card_scheme_a)->SetSelected(choose_a);
    static_cast<SelectableCard*>(m_card_scheme_b)->SetSelected(!choose_a);

    const int scheme = choose_a ? 0 : 1;
    wxGetApp().app_config->set("mouse_scheme", choose_a ? "0" : "1");
    wxGetApp().app_config->save();
    wxGetApp().send_app_message("CP_MOUSE_SCHEME=" + std::to_string(scheme), /*bforce=*/true);
    if (auto* plater = wxGetApp().plater()) {
        auto apply = [&](GLCanvas3D* c) {
            if (!c)
                return;
            c->set_mouse_scheme(scheme);
            if (auto* win = c->get_wxglcanvas()) {
                win->Refresh(false);
#ifdef __WXMSW__
                win->Update();
#endif
            }
        };
        apply(plater->get_view3D_canvas3D());
        apply(plater->get_preview_canvas3D());
        apply(plater->get_assmeble_canvas3D());
        apply(plater->get_current_canvas3D(false));
    }
}

void KBShortcutsDialog::OnSelectTabel(wxCommandEvent &event)
{
    auto                   id = event.GetInt();
    SelectHash::iterator i  = m_hash_selector.begin();
    while (i != m_hash_selector.end()) {
        Select *sel = i->second;
        if (id == sel->m_index) {
            sel->m_tab_button->SetBackgroundColour(StateColor::darkModeColorFor(wxColour("#FFFFFF")));
            sel->m_tab_text->SetBackgroundColour(StateColor::darkModeColorFor(wxColour("#FFFFFF")));
            sel->m_tab_text->SetFont(::Label::Head_13);
            sel->m_tab_button->Refresh();
            sel->m_tab_text->Refresh();

            m_simplebook->SetSelection(id);
        } else {
            sel->m_tab_button->SetBackgroundColour(StateColor::darkModeColorFor(wxColour("#F8F8F8")));
            sel->m_tab_text->SetBackgroundColour(StateColor::darkModeColorFor(wxColour("#F8F8F8")));
            sel->m_tab_text->SetFont(::Label::Body_13);
            sel->m_tab_button->Refresh();
            sel->m_tab_text->Refresh();
        }
        i++;
    }
    wxGetApp().UpdateDlgDarkUI(this);
}

wxWindow *KBShortcutsDialog::create_button(int id, wxString text)
{
    auto tab_button = new wxWindow(m_panel_selects, wxID_ANY, wxDefaultPosition, wxSize( FromDIP(150),  FromDIP(28)), wxTAB_TRAVERSAL);

    wxBoxSizer *sizer = new wxBoxSizer(wxHORIZONTAL);

    sizer->Add(0, 0, 0, wxEXPAND | wxLEFT, FromDIP(22));

    auto stext = new wxStaticText(tab_button, wxID_ANY, text, wxDefaultPosition, wxDefaultSize, 0);
    stext->SetFont(::Label::Body_13);
    stext->SetForegroundColour(wxColour(38, 46, 48));
    stext->Wrap(-1);
    sizer->Add(stext, 1, wxALIGN_CENTER, 0);

    tab_button->Bind(wxEVT_LEFT_DOWN, [this, id](auto &e) {
        auto event = wxCommandEvent(EVT_PREFERENCES_SELECT_TAB);
        event.SetInt(id);
        event.SetEventObject(this);
        wxPostEvent(this, event);
    });

    stext->Bind(wxEVT_LEFT_DOWN, [this, id](wxMouseEvent &e) {
        auto event = wxCommandEvent(EVT_PREFERENCES_SELECT_TAB);
        event.SetInt(id);
        event.SetEventObject(this);
        wxPostEvent(this, event);
    });

    Select *sel                   = new Select;
    sel->m_index                  = id;
    sel->m_tab_button             = tab_button;
    sel->m_tab_text               = stext;
    m_hash_selector[sel->m_index] = sel;

    tab_button->SetSizer(sizer);
    tab_button->Layout();
    return tab_button;
}

void KBShortcutsDialog::on_dpi_changed(const wxRect& suggested_rect)
{
    m_logo_bmp.msw_rescale();
    m_header_bitmap->SetBitmap(m_logo_bmp.bmp());
    msw_buttons_rescale(this, em_unit(), { wxID_OK });

    Layout();
    Fit();
    Refresh();
}

void KBShortcutsDialog::fill_shortcuts()
{
    const std::string& ctrl = GUI::shortkey_ctrl_prefix();
    const std::string& alt = GUI::shortkey_alt_prefix();

    if (wxGetApp().is_editor()) {
        Shortcuts global_shortcuts = {
            // File
            { ctrl + "N", L("New Project") },
            { ctrl + "O", L("Open Project") },
            { ctrl + "S", L("Save Project") },
            { ctrl + "Shift+S", L("Save Project as")},
            // File>Import
            { ctrl + "I", L("Import geometry data from STL/STEP/3MF/OBJ/AMF files") },
            // File>Export
            //{ ctrl + "G", L("Export plate sliced file")},
            { ctrl + "G", L("Export G-code")},
            // Slice plate
            { ctrl + "R", L("Slice plate")},
            // Send to Print
#ifdef __APPLE__
            { L("⌘+Shift+G"), L("Send print")},
#else
            { L("Ctrl+Shift+G"), L("Send print")},
#endif // __APPLE

            // Edit
            { ctrl + "X", L("Cut") },
            { ctrl + "C", L("Copy to clipboard") },
            { ctrl + "V", L("Paste from clipboard") },
            // Configuration
            { ctrl + "P", L("Preferences") },
            //3D control
#ifdef __APPLE__
            { ctrl + "Shift+M", L("Show/Hide 3Dconnexion devices settings dialog") },
#else
            { ctrl + "M", L("Show/Hide 3Dconnexion devices settings dialog") },
#endif // __APPLE
            
            // Switch table page
            { ctrl + "Tab", L("Switch table page")},
            //DEL
            #ifdef __APPLE__
                {"fn+⌫", L("Delete selected")},
            #else
                {L("Del"), L("Delete selected")},
            #endif
            // Help
            { L("Shift+Alt+?"), L("Show keyboard shortcuts list") }
        };
        m_full_shortcuts.push_back({{_L("Global shortcuts"), ""}, global_shortcuts});

        Shortcuts plater_shortcuts = {
            //{ L("Left mouse button"), L("Rotate View") },
            //{ L("Right mouse button"), L("Pan View") },
            { L("Mouse wheel"), L("Zoom View") },
            { "A", L("Arrange all objects") },
            { L("Shift+A"), L("Arrange objects on selected plates") },

            //{ "R", L("Auto orientates selected objects or all objects.If there are selected objects, it just orientates the selected ones.Otherwise, it will orientates all objects in the project.") },
            {L("Shift+R"), L("Auto orientates selected objects or all objects.If there are selected objects, it just orientates the selected ones.Otherwise, it will orientates all objects in the current disk.")},

            {L("Shift+Tab"), L("Collapse/Expand the sidebar")},
            #ifdef __APPLE__
                {L("⌘+Any arrow"), L("Movement in camera space")},
                {L("⌥+Left mouse button"), L("Select a part")},
                {L("⌘+Left mouse button"), L("Select multiple objects")},
            #else
                {L("Ctrl+Any arrow"), L("Movement in camera space")},
                {L("Alt+Left mouse button"), L("Select a part")},
                {L("Ctrl+Left mouse button"), L("Select multiple objects")},

            #endif
            //{L("Shift+Left mouse button"), L("Select objects by rectangle")},
            {L("Arrow Up"), L("Move selection 10 mm in positive Y direction")},
            {L("Arrow Down"), L("Move selection 10 mm in negative Y direction")},
            {L("Arrow Left"), L("Move selection 10 mm in negative X direction")},
            {L("Arrow Right"), L("Move selection 10 mm in positive X direction")},
            {L("Shift+Any arrow"), L("Movement step set to 1 mm")},
            {L("Esc"), L("Deselect all")},
            {"1-9", L("keyboard 1-9: set filament for object/part")},
            {ctrl + "0", L("Camera view - Default")},
            {ctrl + "1", L("Camera view - Top")},
            {ctrl + "2", L("Camera view - Bottom")},
            {ctrl + "3", L("Camera view - Front")},
            {ctrl + "4", L("Camera view - Behind")},
            {ctrl + "5", L("Camera Angle - Left side")},
            {ctrl + "6", L("Camera Angle - Right side")},

            {ctrl + "A", L("Select all objects")},
            {ctrl + "D", L("Delete all")},
            {ctrl + "Z", L("Undo")},
            {ctrl + "Y", L("Redo")},
            { "M", L("Gizmo move") },
            { "S", L("Gizmo scale") },
            { "R", L("Gizmo rotate") },
            { "C", L("Gizmo cut") },
            { "H", L("Gizmo FDM paint-on fuzzy skin") },
            { "F", L("Gizmo Place face on bed") },
            { "L", L("Gizmo SLA support points") },
            { "P", L("Gizmo FDM paint-on seam") },
            { "T", L("Gizmo Text emboss / engrave")},
            { "I", L("Zoom in")},
            { "O", L("Zoom out")},
            { "Tab", L("Switch between Prepare/Preview") },

        };
        m_full_shortcuts.push_back({ { _L("Plater"), "" }, plater_shortcuts });

        Shortcuts gizmos_shortcuts = {
            {L("Esc"), L("Deselect all")},
            {L("Shift+"), L("Move: press to snap by 1mm")},
            #ifdef __APPLE__
                {L("⌘+Mouse wheel"), L("Support/Color Painting: adjust pen radius")},
                {L("⌥+Mouse wheel"), L("Support/Color Painting: adjust section position")},
            #else
		        {L("Ctrl+Mouse wheel"), L("Support/Color Painting: adjust pen radius")},
                {L("Alt+Mouse wheel"), L("Support/Color Painting: adjust section position")},
            #endif
        };
        m_full_shortcuts.push_back({{_L("Gizmo"), ""}, gizmos_shortcuts});

        Shortcuts object_list_shortcuts = {
            {"1-9", L("Set extruder number for the objects and parts") },
            {L("Del"), L("Delete objects, parts, modifiers  ")},
            {L("Esc"), L("Deselect all")},
            {ctrl + "C", L("Copy to clipboard")},
            {ctrl + "V", L("Paste from clipboard")},
            {ctrl + "X", L("Cut")},
            {ctrl + "A", L("Select all objects")},
            {ctrl + "K", L("Clone selected")},
            {ctrl + "Z", L("Undo")},
            {ctrl + "Y", L("Redo")},
            {L("Space"), L("Select the object/part and press space to change the name")},
            {L("Mouse click"), L("Select the object/part and mouse click to change the name")},
            {L("Alt"), L("Mouse clicks can select parts in the assembly model")},
            #ifdef __APPLE__
                {L("F5"), L("Center selected")},
            #else
		        {L("F3"), L("Center selected")},
            #endif
        };
        m_full_shortcuts.push_back({ { _L("Objects List"), "" }, object_list_shortcuts });
    }

    Shortcuts preview_shortcuts = {
        { L("Arrow Up"),    L("Vertical slider - Move active thumb Up")},
        { L("Arrow Down"),  L("Vertical slider - Move active thumb Down")},
        { L("Arrow Left"),  L("Horizontal slider - Move active thumb Left")},
        { L("Arrow Right"), L("Horizontal slider - Move active thumb Right")},
        { "L", L("On/Off one layer mode of the vertical slider")},
        { "C", L("On/Off g-code window")},
        { "Tab", L("Switch between Prepare/Preview") },
        {L("Shift+Any arrow"), L("Move slider 5x faster")},
        {L("Shift+Mouse wheel"), L("Move slider 5x faster")},
        #ifdef __APPLE__
            {L("⌘+Any arrow"), L("Move slider 5x faster")},
            {L("⌘+Mouse wheel"), L("Move slider 5x faster")},
        #else
		    {L("Ctrl+Any arrow"), L("Move slider 5x faster")},
		    {L("Ctrl+Mouse wheel"), L("Move slider 5x faster")},
       #endif
        { L("Home"),        L("Horizontal slider - Move to start position")},
        { L("End"),         L("Horizontal slider - Move to last position")},
    };
    m_full_shortcuts.push_back({ { _L("Preview"), "" }, preview_shortcuts });
}

wxPanel* KBShortcutsDialog::create_page(wxWindow* parent, const ShortcutsItem& shortcuts, const wxFont& font, const wxFont& bold_font)
{
    wxPanel* main_page = new wxPanel(parent);
    main_page->SetBackgroundColour(this->GetBackgroundColour());  
    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);

    wxScrolledWindow* scrollable_panel = new wxScrolledWindow(main_page);
    wxGetApp().UpdateDarkUI(scrollable_panel);
    scrollable_panel->SetScrollbars(20, 20, 50, 50);
    scrollable_panel->SetBackgroundColour(main_page->GetBackgroundColour());

    wxBoxSizer* scrollable_panel_sizer = new wxBoxSizer(wxVERTICAL);

    if (shortcuts.first.first == _L("Plater")) {
        m_mouse_cards_host = new wxPanel(scrollable_panel);
        m_mouse_cards_host->SetBackgroundColour(scrollable_panel->GetBackgroundColour());
        m_mouse_cards_host->SetSizer(new wxBoxSizer(wxHORIZONTAL));

        create_mouse_scheme_cards(m_mouse_cards_host);

        int saved = 0;
        try {
            saved = wxAtoi(wxGetApp().app_config->get("mouse_scheme"));
        } catch (...) {}
        select_scheme(saved == 0);

        scrollable_panel_sizer->Add(m_mouse_cards_host, 0, wxEXPAND | wxALL, FromDIP(8));

        // 分割线
#ifdef __WXMSW__
        auto* line = new wxStaticLine(scrollable_panel);
        scrollable_panel_sizer->Add(line, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(8));
#else
        scrollable_panel_sizer->AddSpacer(FromDIP(6));
#endif
    }

    if (!shortcuts.first.second.empty()) {
        main_sizer->AddSpacer(FromDIP(10));
        wxBoxSizer* info_sizer = new wxBoxSizer(wxHORIZONTAL);
        info_sizer->AddStretchSpacer();
        info_sizer->Add(new wxStaticText(main_page, wxID_ANY, shortcuts.first.second), 0);
        info_sizer->AddStretchSpacer();
        main_sizer->Add(info_sizer, 0, wxEXPAND);
        main_sizer->AddSpacer(FromDIP(10));
    }

    int items_count = (int) shortcuts.second.size();
    //wxScrolledWindow *scrollable_panel = new wxScrolledWindow(main_page);
    //wxGetApp().UpdateDarkUI(scrollable_panel);
    //scrollable_panel->SetScrollbars(20, 20, 50, 50);
    //scrollable_panel->SetInitialSize(wxSize(FromDIP(850), FromDIP(450)));

    //wxBoxSizer *     scrollable_panel_sizer = new wxBoxSizer(wxVERTICAL);
    wxFlexGridSizer *grid_sizer             = new wxFlexGridSizer(items_count, 2, FromDIP(10), FromDIP(20));
    // Make the description column grow to take available width to avoid
    // unintended wrapping on Linux with wider fonts.
    grid_sizer->AddGrowableCol(1, 1);
    grid_sizer->SetFlexibleDirection(wxBOTH);
    grid_sizer->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_SPECIFIED);

    // Collect description labels to update wrap width on resize (Linux/GTK).
    std::vector<wxStaticText*> desc_labels;
    desc_labels.reserve(items_count);

    // Measure the maximum width needed by the first (key) column to avoid
    // GTK/Pango wrapping the last character when the column is squeezed.
    int max_key_width = 0;
    {
        wxClientDC dc(scrollable_panel);
        dc.SetFont(bold_font);
        for (int i = 0; i < items_count; ++i) {
            const auto& shortcut = shortcuts.second[i].first;
            wxSize      ext;
            dc.GetTextExtent(_(shortcut), &ext.x, &ext.y);
            max_key_width = std::max(max_key_width, ext.x);
        }
        // Add a small safety padding to account for DPI rounding.
        max_key_width += FromDIP(4);
    }

    for (int i = 0; i < items_count; ++i) {
        const auto &[shortcut, description] = shortcuts.second[i];
        auto key                            = new wxStaticText(scrollable_panel, wxID_ANY, _(shortcut));
        key->SetForegroundColour(wxColour(50, 58, 61));
        key->SetFont(bold_font);
        // Prevent any wrapping and ensure enough width for the longest key.
        key->Wrap(-1);
        key->SetMinSize(wxSize(max_key_width, -1));
        grid_sizer->Add(key, 0, wxALIGN_CENTRE_VERTICAL | wxRIGHT, FromDIP(8));

        auto desc = new wxStaticText(scrollable_panel, wxID_ANY, _(description));
        desc->SetFont(font);
        desc->SetForegroundColour(wxColour(50, 58, 61));
        // On Linux/GTK, wxStaticText may still wrap when the sizer gives
        // a smaller width. Force a very large wrap width to effectively
        // disable wrapping in normal dialog sizes.
        desc->Wrap(10000);
        grid_sizer->Add(desc, 0, wxALIGN_CENTRE_VERTICAL | wxEXPAND);
        desc_labels.push_back(desc);
    }

    scrollable_panel_sizer->Add(grid_sizer, 1, wxEXPAND | wxALL, FromDIP(20));
    scrollable_panel->SetSizer(scrollable_panel_sizer);

    // Dynamically adjust wrap width on resize so labels keep single-line
    // appearance when there is enough space.
    scrollable_panel->Bind(wxEVT_SIZE, [desc_labels, scrollable_panel](wxSizeEvent& e) {
        const int w = e.GetSize().GetWidth();
        // Reserve space for the first column and margins.
        const int wrap = std::max(w - scrollable_panel->FromDIP(260), scrollable_panel->FromDIP(220));
        for (auto* st : desc_labels) {
            if (st)
                st->Wrap(wrap);
        }
        e.Skip();
    });

    main_sizer->Add(scrollable_panel, 1, wxEXPAND);
    main_page->SetSizer(main_sizer);

    return main_page;
}

} // namespace GUI
} // namespace Slic3r
