#ifndef slic3r_GUI_ParamsDialog_hpp_
#define slic3r_GUI_ParamsDialog_hpp_

#include <wx/wx.h>
#include <wx/intl.h>
#include <wx/collpane.h>

#include "GUI_Utils.hpp"
#include "wxExtensions.hpp"

namespace Slic3r { 
namespace GUI {

wxDECLARE_EVENT(EVT_MODIFY_FILAMENT, SimpleEvent);

class FilamentInfomation : public wxObject
{
public:
    std::string filament_id;
    std::string filament_name;
};

class ParamsPanel;

class ParamsDialog : public DPIDialog
{
public:
    ParamsDialog(wxWindow * parent);

    ParamsPanel * panel() { return m_panel; }

    void Popup();

    void set_editing_filament_id(std::string id) { m_editing_filament_id = id; }

protected:
    void on_dpi_changed(const wxRect& suggested_rect) override;

private:
    std::string       m_editing_filament_id;
    ParamsPanel * m_panel;
    wxWindowDisabler *m_winDisabler = nullptr;
};

class PrinterDialog : public DPIDialog
{
public:
    PrinterDialog(wxWindow* parent);

    void Popup();

protected:
    void on_dpi_changed(const wxRect& suggested_rect) override;

private:
    wxWindowDisabler* m_winDisabler = nullptr;
};

class CusTitlePanel :public wxPanel
{
public:
    CusTitlePanel(wxWindow* parent, const wxString& title);
    void OnPaint(wxPaintEvent& event);
    void OnMouseLeftDown(wxMouseEvent& event);
    void OnMouseLeftUp(wxMouseEvent& event);
    void OnMouseMove(wxMouseEvent& event);

    void setBgColor(const wxColor& color);
    void setTitle(const wxString& title);
protected:

private:
    wxColor m_bgColor = wxColor("#4b4b4d");
    wxString m_title;
    wxBitmap m_closeBitmap;
    wxBitmap m_closeBitmapHover;
    bool m_isCloseButtonHover;
    bool m_dragging;
    wxPoint m_dragStartPos;
    wxSizer* m_MainSizer = nullptr;
};

class CusDialog : public DPIDialog
{
public:
    CusDialog(wxWindow* parent, wxWindowID id = wxID_ANY,
        const wxString& title = wxString(),
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxDEFAULT_DIALOG_STYLE,
        const wxString& name = wxASCII_STR(wxDialogNameStr));
    void Popup();

    void setTitleHeight(int height);
    wxPanel* titlePanel();
    void addToolButton(wxPanel* btn);

protected:
    void OnPaint(wxPaintEvent& event);
    void on_dpi_changed(const wxRect& suggested_rect) override;
    
    wxPanel* m_ContentPanel = nullptr;
    wxWindowDisabler* m_winDisabler = nullptr;
    CusTitlePanel* m_TitlePanel = nullptr;
    wxStaticText* m_TitleText = nullptr;

private:
    void eventFilter();

    int m_TitleHeight = 30;
    bool m_dragging = false;
    wxPoint m_dragStartPos;
};

} // namespace GUI
} // namespace Slic3r

#endif
