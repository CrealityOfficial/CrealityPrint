#ifndef slic3r_GUI_ComboBox_hpp_
#define slic3r_GUI_ComboBox_hpp_

#include "TextInput.hpp"
#include "DropDown.hpp"

#define CB_NO_DROP_ICON DD_NO_CHECK_ICON
#define CB_NO_TEXT DD_NO_TEXT

class ComboBox : public wxWindowWithItems<TextInput, wxItemContainer>
{
    std::vector<wxString>         texts;
    std::vector<wxString>         tips;
    std::vector<wxBitmap>         icons;
    std::vector<void *>           datas;
    std::vector<wxClientDataType> types;
    std::vector<bool>           group_headers;
    std::vector<int>              indents;
    std::vector<char>             bolds;

    DropDown               drop;
    bool     drop_down = false;
    bool     text_off = false;
    static ComboBox *active_drop_down;

public:
    std::vector<wxBitmap> icons_backup;
    ComboBox(wxWindow *      parent,
             wxWindowID      id,
             const wxString &value     = wxEmptyString,
             const wxPoint & pos       = wxDefaultPosition,
             const wxSize &  size      = wxDefaultSize,
             int             n         = 0,
             const wxString  choices[] = NULL,
             long            style     = 0,
             int             flags     = wxPU_CONTAINS_CONTROLS);

    DropDown & GetDropDown() { return drop; }
    static void DismissActiveDropDown();

    void EnableAutoPopupDirection(bool enable = true);

    virtual bool SetFont(wxFont const & font) override;

public:
    int Append(const wxString &item, const wxBitmap &bitmap = wxNullBitmap);

    int AppendGroupHeader(const wxString &text);

    int Append(const wxString &item, const wxBitmap &bitmap, void *clientData);

    unsigned int GetCount() const override;

    int  GetSelection() const override;

    void SetSelection(int n) override;

    void SelectAndNotify(int n);

    virtual void Rescale() override;

    wxString GetValue() const;
    void     SetValue(const wxString &value);

    void SetLabel(const wxString &label) override;
    wxString GetLabel() const override;

    void SetTextLabel(const wxString &label);
    wxString GetTextLabel() const;

    wxString GetString(unsigned int n) const override;
    void     SetString(unsigned int n, wxString const &value) override;

    wxString GetItemTooltip(unsigned int n) const;
    void     SetItemTooltip(unsigned int n, wxString const &value);

    wxBitmap GetItemBitmap(unsigned int n);
    void     GetItemBitmaps(std::vector<wxBitmap>& out_icons);
    void     SetItemBitmap(unsigned int n, wxBitmap const &bitmap);
    bool     is_drop_down(){return drop_down;}
    void     DeleteOneItem(unsigned int pos) { DoDeleteOneItem(pos); }
    void     ForceDropdownOpen();

    bool     IsGroupHeader(unsigned int n) const;
    // Hierarchical rendering: indents item `n` by `level` steps in the drop-down
    // list. Enabling it for any item switches the whole list to indented layout.
    void     SetItemIndent(unsigned int n, int level);
    void     EnableItemIndents(bool enable, int step_dip = 16);

    // Renders item `n` in bold in the drop-down list, for group headers.
    void     SetItemBold(unsigned int n, bool bold);

    // Opens the drop-down under `rect` (screen coords) with the given width,
    // instead of deriving both from this combobox. `dpi_reference` supplies
    // the monitor DPI when this combobox is hosted by a hidden window.
    void     SetDropDownAnchor(const wxRect &rect, int width = 0, wxWindow* dpi_reference = nullptr);
protected:
    virtual int  DoInsertItems(const wxArrayStringsAdapter &items,
                               unsigned int                 pos,
                               void **                      clientData,
                               wxClientDataType             type) override;
    virtual void DoClear() override;

    void DoDeleteOneItem(unsigned int pos) override;

    void *DoGetItemClientData(unsigned int n) const override;
    void  DoSetItemClientData(unsigned int n, void *data) override;
    
    void OnEdit() override;

    void sendComboBoxEvent();

#ifdef __WIN32__
    WXLRESULT MSWWindowProc(WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam) override;
#endif

private:

    // some useful events
    void mouseDown(wxMouseEvent &event);
    void mouseWheelMoved(wxMouseEvent &event);
    void keyDown(wxKeyEvent &event);
    void dismissDropDown();
    void onMove(wxMoveEvent &event);
    int nextSelectableItem(int direction) const;

    DECLARE_EVENT_TABLE()
};

#endif // !slic3r_GUI_ComboBox_hpp_
