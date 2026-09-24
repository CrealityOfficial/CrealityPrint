#ifndef slic3r_GUI_DropDown_hpp_
#define slic3r_GUI_DropDown_hpp_

#include <boost/date_time/posix_time/posix_time.hpp>
#include <wx/stattext.h>
#include "../wxExtensions.hpp"
#include "StateHandler.hpp"
#include "PopupWindow.hpp"

#define DD_NO_CHECK_ICON    0x0001
#define DD_NO_TEXT          0x0002
#define DD_STYLE_MASK       0x0003

wxDECLARE_EVENT(EVT_DISMISS, wxCommandEvent);

class DropDown : public PopupWindow
{
public:
    enum class PopupDirection
    {
        Auto,
        Down,
        Up
    };

private:
    std::vector<wxString> &       texts;
    std::vector<wxString> &       tips;
    std::vector<wxBitmap> &     icons;
    std::vector<bool> *         group_headers = nullptr;
    bool                          need_sync  = false;
    int                         selection = -1;
    int                         hover_item = -1;

    double radius = 0;
    bool   use_content_width = false;
    bool   limit_max_content_width = false;
    bool   align_icon        = false;
    bool   text_off          = false;

    wxSize textSize;
    wxSize iconSize;
    wxSize rowSize;

    StateHandler state_handler;
    StateColor   text_color;
    StateColor   border_color;
    StateColor   selector_border_color;
    StateColor   selector_background_color;
    ScalableBitmap check_bitmap;

    bool pressedDown = false;
    boost::posix_time::ptime dismissTime;
    wxPoint                  offset; // x not used
    wxPoint                  dragStart;
    int                      m_drapDownGap = 6;
    size_t                   m_max_visible_items = 15;
    PopupDirection           m_popup_direction = PopupDirection::Auto;

    // Optional per-item indent levels, owned by the ComboBox. Used to render
    // hierarchical lists (e.g. brand > series).
    const std::vector<int> * indents = nullptr;
    int                      indent_step = 0;
    int                      indent_step_dip = 16;

    // Optional per-item bold flags, owned by the ComboBox. Used to emphasize
    // group headers (e.g. brand names).
    const std::vector<char> * bolds = nullptr;

    // Optional explicit anchor (screen coords) and width, used when the list must
    // not be positioned/sized from the owning combobox. Empty rect = disabled.
    wxRect                   m_anchor_rect;
    int                      m_anchor_width = 0;
    wxWindow*                m_dpi_reference = nullptr;
    // Height cap computed by autoPosition() when the list does not fit on screen.
    // messureSize() must honour it, otherwise a later re-measure would restore the
    // full height, pushing the list off-screen and hiding the scrollbar.
    int                      m_anchor_max_height = 0;
    // Target rect computed by autoPosition(), re-applied by Popup() once the
    // window is actually shown. Empty when no explicit anchor is in use.
    wxRect                   m_anchor_target;

    // Scrollbar thumb dragging.
    bool                     dragging_scrollbar = false;
    int                      scrollbar_grab_dy = 0;

public:
    DropDown(std::vector<wxString> &texts,
             std::vector<wxString> &tips,
             std::vector<wxBitmap> &icons);
    
    DropDown(std::vector<wxString> &texts,
             std::vector<wxString> &tips,
             std::vector<wxBitmap> &icons,
             std::vector<bool> *group_headers);
    
    DropDown(wxWindow *     parent,
             std::vector<wxString> &texts,
             std::vector<wxString> &tips,
             std::vector<wxBitmap> &icons,
             long           style     = 0);
    
    void Create(wxWindow* parent, long style = 0, int flags = wxPU_CONTAINS_CONTROLS);

    // Returns the cumulative Y position for an item, accounting for group header extra spacing
    int  GetItemY(int index) const;
    int  GetContentHeight() const;

public:
    void Invalidate(bool clear = false);

    int GetSelection() const { return selection; }

    void SetSelection(int n);

    wxString GetValue() const;
    void     SetValue(const wxString &value);
    void     setDrapDownGap(int drapDownGap);

public:
    void SetCornerRadius(double radius);

    void SetBorderColor(StateColor const & color);

    void SetSelectorBorderColor(StateColor const & color);

    void SetTextColor(StateColor const &color);

    void SetSelectorBackgroundColor(StateColor const &color);

    void SetUseContentWidth(bool use, bool limit_max_content_width = false);

    void SetAlignIcon(bool align);
    
    void SetMaxVisibleItems(size_t max_items);

    void SetPopupDirection(PopupDirection direction);

    bool IsGroupHeader(int index) const;
    // Enables hierarchical rendering. `levels` is not copied and must outlive
    // this window; pass nullptr to disable.
    void SetIndents(const std::vector<int> *levels, int step_dip = 16);

    // Renders flagged items in bold. `flags` is not copied and must outlive this
    // window; pass nullptr to disable.
    void SetBolds(const std::vector<char> *flags);

    // Positions the list under `rect` (screen coords) and forces its width,
    // instead of deriving both from the owning combobox. `dpi_reference` is
    // the visible trigger whose monitor DPI must be used by a hidden popup.
    // Pass an empty rect to go back to the default behaviour.
    void SetAnchor(const wxRect &rect, int width = 0, wxWindow* dpi_reference = nullptr);
    
public:
    void Rescale();

    bool HasDismissLongTime();

    void Popup(wxWindow *focus = nullptr) override;
    
protected:
    void OnDismiss() override;

private:
    void paintEvent(wxPaintEvent& evt);
    void paintNow();

    void render(wxDC& dc);

    friend class ComboBox;
    void messureSize();
    void autoPosition();

    // some useful events
    void mouseDown(wxMouseEvent& event);
    void mouseReleased(wxMouseEvent &event);
    void mouseCaptureLost(wxMouseCaptureLostEvent &event);
    void mouseMove(wxMouseEvent &event);
    void mouseWheelMoved(wxMouseEvent &event);

    void sendDropDownEvent();

    // Total pixel height of all rows.
    int  contentHeight() const;
    // Geometry of the scrollbar track / thumb, empty when no scrollbar is needed.
    bool scrollbarRects(wxRect &track, wxRect &thumb) const;
    // Clamps and applies a new vertical offset, returns true when it changed.
    bool setScrollOffset(int y);
    // Maps a thumb top position back to a content offset.
    void scrollToThumbTop(int thumb_top);
    int  indentFor(size_t i) const;
    bool isBold(size_t i) const;
    int  dip(int value) const;


    DECLARE_EVENT_TABLE()
};

#endif // !slic3r_GUI_DropDown_hpp_
