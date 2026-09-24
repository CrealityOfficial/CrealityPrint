#include "ComboBox.hpp"
#include "Label.hpp"

#include <wx/dcgraph.h>

ComboBox *ComboBox::active_drop_down = nullptr;

BEGIN_EVENT_TABLE(ComboBox, TextInput)

EVT_LEFT_DOWN(ComboBox::mouseDown)
EVT_LEFT_DCLICK(ComboBox::mouseDown)
//EVT_MOUSEWHEEL(ComboBox::mouseWheelMoved)
EVT_KEY_DOWN(ComboBox::keyDown)

// catch paint events
END_EVENT_TABLE()

/*
 * Called by the system of by wxWidgets when the panel needs
 * to be redrawn. You can also trigger this call by
 * calling Refresh()/Update().
 */

static wxWindow *GetScrollParent(wxWindow *pWindow)
{
    wxWindow *pWin = pWindow;
    while (pWin->GetParent()) {
        auto pWin2 = pWin->GetParent();
        if (auto top = dynamic_cast<wxScrollHelper *>(pWin2))
            return dynamic_cast<wxWindow *>(pWin);
        pWin = pWin2;
    }
    return nullptr;
}

ComboBox::ComboBox(wxWindow *parent,
                   wxWindowID      id,
                   const wxString &value,
                   const wxPoint & pos,
                   const wxSize &  size,
                   int             n,
                   const wxString  choices[],
                   long            style,
                   int flags )
    : drop(texts, tips, icons, &group_headers)
{
    if (style & wxCB_READONLY)
        style |= wxRIGHT;
    text_off = style & CB_NO_TEXT;
    TextInput::Create(parent, "", value, (style & CB_NO_DROP_ICON) ? "" : "drop_down", pos, size,
                      style | wxTE_PROCESS_ENTER);
    drop.Create(this, style & DD_STYLE_MASK, flags);

    if (style & wxCB_READONLY) {
        GetTextCtrl()->Hide();
        TextInput::SetFont(Label::Body_13);
        TextInput::SetBorderColor(StateColor(std::make_pair(0xDBDBDB, (int) StateColor::Disabled),
            std::make_pair(0x15BF59, (int) StateColor::Hovered),
            std::make_pair(0xDBDBDB, (int) StateColor::Normal)));
        TextInput::SetBackgroundColor(StateColor(std::make_pair(0xF0F0F1, (int) StateColor::Disabled),
                       std::make_pair(*wxWHITE, (int) StateColor::Focused), // ORCA updated background color for focused item
            std::make_pair(*wxWHITE, (int) StateColor::Normal)));
        TextInput::SetLabelColor(StateColor(
            std::make_pair(wxColour("#ACACAC"), (int) StateColor::Disabled), // ORCA: Use same color for disabled text on combo boxes
            std::make_pair(0x000000, (int) StateColor::Normal)));
    }
    if (auto scroll = GetScrollParent(this))
        scroll->Bind(wxEVT_MOVE, &ComboBox::onMove, this);
    drop.Bind(wxEVT_COMBOBOX, [this](wxCommandEvent &e) {
        SetSelection(e.GetInt());
        e.SetEventObject(this);
        e.SetId(GetId());
        GetEventHandler()->ProcessEvent(e);
    });
    drop.Bind(EVT_DISMISS, [this](auto &) {
        if (active_drop_down == this)
            active_drop_down = nullptr;
        drop_down = false;
        wxCommandEvent e(wxEVT_COMBOBOX_CLOSEUP);
        GetEventHandler()->ProcessEvent(e);
    });
    Bind(wxEVT_DESTROY, [this](wxWindowDestroyEvent &e) {
        if (active_drop_down == this)
            active_drop_down = nullptr;
        e.Skip();
    });
    for (int i = 0; i < n; ++i) Append(choices[i]);
}

void ComboBox::DismissActiveDropDown()
{
    if (active_drop_down)
        active_drop_down->dismissDropDown();
}

void ComboBox::EnableAutoPopupDirection(bool enable)
{
    drop.SetPopupDirection(enable ? DropDown::PopupDirection::Auto : DropDown::PopupDirection::Down);
}

int ComboBox::GetSelection() const { return drop.GetSelection(); }

void ComboBox::SetSelection(int n)
{
    if (n == drop.selection)
        return;
    drop.SetSelection(n);
    SetLabel(drop.GetValue());
    if (drop.selection >= 0 && drop.iconSize.y > 0)
        SetIcon(icons[drop.selection].IsNull() ? create_scaled_bitmap("drop_down", this, 16): icons[drop.selection]); // ORCA fix combo boxes without arrows
}
void ComboBox::SelectAndNotify(int n) { 
    SetSelection(n);
    sendComboBoxEvent();
}

void ComboBox::Rescale()
{
    // SetSelection() may replace the named scalable arrow with an anonymous
    // wxBitmap. Recreate that bitmap for the destination monitor before the base
    // class rescales, otherwise the 4K arrow survives on a 1080p display.
    if (drop.selection >= 0 && size_t(drop.selection) < icons.size() && drop.iconSize.y > 0) {
        const wxBitmap bitmap = icons[drop.selection].IsNull()
            ? create_scaled_bitmap("drop_down", this, 16)
            : icons[drop.selection];
        SetIconBitmapWithoutRescale(bitmap);
    }
    TextInput::Rescale();
    drop.Rescale();
}

wxString ComboBox::GetValue() const
{
    return drop.GetSelection() >= 0 ? drop.GetValue() : GetLabel();
}

void ComboBox::SetValue(const wxString &value)
{
    drop.SetValue(value);
    SetLabel(value);
    if (drop.selection >= 0 && drop.iconSize.y > 0)
        SetIcon(icons[drop.selection].IsNull() ? create_scaled_bitmap("drop_down", this, 16): icons[drop.selection]); // ORCA fix combo boxes without arrows
}

void ComboBox::SetLabel(const wxString &value)
{
    if (GetTextCtrl()->IsShown() || text_off)
        GetTextCtrl()->SetValue(value);
    else
        TextInput::SetLabel(value);
}

wxString ComboBox::GetLabel() const
{
    if (GetTextCtrl()->IsShown() || text_off)
        return GetTextCtrl()->GetValue();
    else
        return TextInput::GetLabel();
}

void ComboBox::SetTextLabel(const wxString& label)
{
    TextInput::SetLabel(label);
}

wxString ComboBox::GetTextLabel() const
{
    return TextInput::GetLabel();
}

bool ComboBox::SetFont(wxFont const& font)
{
    if (GetTextCtrl() && GetTextCtrl()->IsShown())
        return GetTextCtrl()->SetFont(font);
    else
        return TextInput::SetFont(font);
}

int ComboBox::Append(const wxString &item, const wxBitmap &bitmap)
{
    return Append(item, bitmap, nullptr);
}

int ComboBox::Append(const wxString &item,
                     const wxBitmap &bitmap,
                     void *          clientData)
{
    texts.push_back(item);
    tips.push_back(wxString{});
    icons.push_back(bitmap);
    datas.push_back(clientData);
    types.push_back(wxClientData_None);
    indents.push_back(0);
    bolds.push_back(0);
    group_headers.push_back(false);
    drop.Invalidate();
    return texts.size() - 1;
}

int ComboBox::AppendGroupHeader(const wxString &text)
{
    texts.push_back(text);
    tips.push_back(wxString{});
    icons.push_back(wxNullBitmap);
    datas.push_back(nullptr);
    types.push_back(wxClientData_None);
    group_headers.push_back(true);
    indents.push_back(0);
    bolds.push_back(0);
    drop.Invalidate();
    return texts.size() - 1;
}

void ComboBox::DoClear()
{
    texts.clear();
    tips.clear();
    icons.clear();
    datas.clear();
    types.clear();
    group_headers.clear();
    indents.clear();
    bolds.clear();
    //icons_backup.clear();
    drop.Invalidate(true);
    // SetIcon() calls Rescale(), which can restore the selected item's bitmap.
    // Reset the items and selection first so pooled controls cannot retain it.
    SetIcon("drop_down");
}

void ComboBox::DoDeleteOneItem(unsigned int pos)
{
    if (pos >= texts.size()) return;
    texts.erase(texts.begin() + pos);
    tips.erase(tips.begin() + pos);
    icons.erase(icons.begin() + pos);
    datas.erase(datas.begin() + pos);
    types.erase(types.begin() + pos);
    if (pos < group_headers.size())
        group_headers.erase(group_headers.begin() + pos);
    indents.erase(indents.begin() + pos);
    bolds.erase(bolds.begin() + pos);
    drop.Invalidate(true);
}

void ComboBox::SetItemIndent(unsigned int n, int level)
{
    if (n >= indents.size()) return;
    indents[n] = std::max(0, level);
    drop.Invalidate();
}

void ComboBox::EnableItemIndents(bool enable, int step_dip)
{
    drop.SetIndents(enable ? &indents : nullptr, step_dip);
}

void ComboBox::SetItemBold(unsigned int n, bool bold)
{
    if (n >= bolds.size()) return;
    bolds[n] = bold ? 1 : 0;
    // Bold flags are only consulted once the list knows about them.
    drop.SetBolds(&bolds);
    drop.Invalidate();
}

void ComboBox::SetDropDownAnchor(const wxRect &rect, int width, wxWindow* dpi_reference)
{
    drop.SetAnchor(rect, width, dpi_reference);
}

unsigned int ComboBox::GetCount() const { return texts.size(); }

wxString ComboBox::GetString(unsigned int n) const
{
    return n < texts.size() ? texts[n] : wxString{};
}

void ComboBox::SetString(unsigned int n, wxString const &value)
{
    if (n >= texts.size()) return;
    texts[n]  = value;
    drop.Invalidate();
    if (n == drop.GetSelection()) SetLabel(value);
}

wxString ComboBox::GetItemTooltip(unsigned int n) const
{
    if (n >= texts.size()) return wxString();
    return tips[n];
}

void ComboBox::SetItemTooltip(unsigned int n, wxString const &value) {
    if (n >= texts.size()) return;
    tips[n] = value;
    if (n == drop.GetSelection()) drop.SetToolTip(value);
}

wxBitmap ComboBox::GetItemBitmap(unsigned int n) { return icons[n]; }

void ComboBox::GetItemBitmaps(std::vector<wxBitmap>& out_icons) {out_icons = std::move(icons); }

void ComboBox::SetItemBitmap(unsigned int n, wxBitmap const &bitmap)
{
    if (n >= texts.size()) return;
    icons[n] = bitmap;
    drop.Invalidate();
}

int ComboBox::DoInsertItems(const wxArrayStringsAdapter &items,
                            unsigned int                 pos,
                            void **                      clientData,
                            wxClientDataType             type)
{
    if (pos > texts.size()) return -1;
    for (int i = 0; i < items.GetCount(); ++i) {
        texts.insert(texts.begin() + pos, items[i]);
        tips.insert(tips.begin() + pos, wxString{});
        icons.insert(icons.begin() + pos, wxNullBitmap);
        datas.insert(datas.begin() + pos, clientData ? clientData[i] : NULL);
        types.insert(types.begin() + pos, type);
        group_headers.insert(group_headers.begin() + pos, false);
        indents.insert(indents.begin() + pos, 0);
        bolds.insert(bolds.begin() + pos, 0);
        ++pos;
    }
    drop.Invalidate(true);
    return pos - 1;
}

void *ComboBox::DoGetItemClientData(unsigned int n) const { return n < texts.size() ? datas[n] : NULL; }

bool ComboBox::IsGroupHeader(unsigned int n) const
{
    return n < group_headers.size() && group_headers[n];
}

void ComboBox::DoSetItemClientData(unsigned int n, void *data)
{
    if (n < texts.size())
        datas[n] = data;
}

wxDECLARE_EVENT(EVT_DISMISS, wxCommandEvent);

class SearchObjectDialog2 : public PopupWindow
{
public:
    SearchObjectDialog2(wxWindow* parent);
    ~SearchObjectDialog2();

    void Popup(wxPoint position = wxDefaultPosition);
    void Dismiss();

    void update_list();

public:
    int       em;
    const int POPUP_WIDTH  = 41;
    const int POPUP_HEIGHT = 45;

    wxColour m_text_color;
    wxColour m_bg_color;
    wxColour m_thumb_color;
    wxColour m_bold_color;

    wxBoxSizer* m_sizer_body{nullptr};
    wxBoxSizer* m_sizer_main{nullptr};
    wxBoxSizer* m_sizer_border{nullptr};

    wxBoxSizer* m_sizer_ctrl{nullptr};

    wxWindow* m_border_panel{nullptr};
    wxWindow* m_client_panel{nullptr};
    wxWindow* m_listPanel{nullptr};

    DECLARE_EVENT_TABLE()
};

void ComboBox::ForceDropdownOpen()
{
    if (!IsEnabled() || drop_down)
        return;

    DismissActiveDropDown();
    drop.autoPosition();
    drop_down = true;
    active_drop_down = this;
    drop.Popup(&drop);

    wxCommandEvent e(wxEVT_COMBOBOX_DROPDOWN);
    GetEventHandler()->ProcessEvent(e);
}

void ComboBox::mouseDown(wxMouseEvent &event)
{
    SetFocus();
    if (drop_down) {
        dismissDropDown();
    } else if (drop.HasDismissLongTime()) {
        DismissActiveDropDown();
        drop.autoPosition();
        drop_down = true;
        active_drop_down = this;
        //drop.Raise();
        drop.Popup();
        
        wxCommandEvent e(wxEVT_COMBOBOX_DROPDOWN);
        GetEventHandler()->ProcessEvent(e);
    }
}

int ComboBox::nextSelectableItem(int direction) const
{
    int next = GetSelection() + direction;
    while (next >= 0 && next < static_cast<int>(GetCount()) && IsGroupHeader(next))
        next += direction;
    return next >= 0 && next < static_cast<int>(GetCount()) ? next : -1;
}

void ComboBox::mouseWheelMoved(wxMouseEvent &event)
{
    event.Skip();
    if (drop_down) {
        dismissDropDown();
        return;
    }
    auto delta = event.GetWheelRotation() < 0 ? 1 : -1;
    const int n = nextSelectableItem(delta);
    if (n >= 0) {
        SetSelection((int) n);
        sendComboBoxEvent();
    }
}


void ComboBox::keyDown(wxKeyEvent& event)
{
    switch (event.GetKeyCode()) {
        case WXK_RETURN:
        case WXK_SPACE:
            if (drop_down) {
                dismissDropDown();
            } else if (drop.HasDismissLongTime()) {
                DismissActiveDropDown();
                drop.autoPosition();
                drop_down = true;
                active_drop_down = this;
                drop.Popup();
                wxCommandEvent e(wxEVT_COMBOBOX_DROPDOWN);
                GetEventHandler()->ProcessEvent(e);
            }
            break;
        case WXK_UP:
        case WXK_DOWN:
        case WXK_LEFT:
        case WXK_RIGHT: {
            const int direction = (event.GetKeyCode() == WXK_UP || event.GetKeyCode() == WXK_LEFT) ? -1 : 1;
            const int next = nextSelectableItem(direction);
            if (next >= 0) {
                SetSelection(next);
                sendComboBoxEvent();
            }
            break;
        }
        case WXK_TAB:
            HandleAsNavigationKey(event);
            break;
        default:
            event.Skip();
            break;
    }
}

void ComboBox::onMove(wxMoveEvent &event)
{
    event.Skip();
    if (drop_down)
        dismissDropDown();
}

void ComboBox::dismissDropDown()
{
    if (!drop_down && active_drop_down != this)
        return;
    drop.DismissAndNotify();
}

void ComboBox::OnEdit()
{
    auto value = GetTextCtrl()->GetValue();
    SetValue(value);
}

#ifdef __WIN32__

WXLRESULT ComboBox::MSWWindowProc(WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam)
{
    if (nMsg == WM_GETDLGCODE) {
        return DLGC_WANTALLKEYS;
    }
    return TextInput::MSWWindowProc(nMsg, wParam, lParam);
}

#endif

void ComboBox::sendComboBoxEvent()
{
    wxCommandEvent event(wxEVT_COMBOBOX, GetId());
    event.SetEventObject(this);
    event.SetInt(drop.GetSelection());
    event.SetString(drop.GetValue());
    GetEventHandler()->ProcessEvent(event);
}
