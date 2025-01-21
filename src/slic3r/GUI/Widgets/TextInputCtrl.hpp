#ifndef TEXTINPUTCTRL_H
#define TEXTINPUTCTRL_H

#include <wx/textctrl.h>
#include "StaticBox.hpp"

class TextInputCtrl : public wxNavigationEnabled<StaticBox>
{
public: 
    wxTextCtrl * text_ctrl = nullptr;
    StateColor     text_color;
public:
    TextInputCtrl(wxWindow *     parent,
              int id,
              wxString       text,
              const wxPoint &pos   = wxDefaultPosition,
              const wxSize & size  = wxDefaultSize,
              long           style = wxBORDER_NONE);

public:   
    void paintEvent(wxPaintEvent& evt);
    void Rescale();
    void OnSize(wxSizeEvent& event);

    DECLARE_EVENT_TABLE()
};

#endif // !slic3r_GUI_TextInput_hpp_
