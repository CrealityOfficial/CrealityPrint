#include "TextInputCtrl.hpp"
#include "Label.hpp"
#include "TextCtrl.h"
#include "slic3r/GUI/Widgets/Label.hpp"

#include <wx/dcclient.h>
#include <wx/dcgraph.h>

BEGIN_EVENT_TABLE(TextInputCtrl, wxPanel)

EVT_PAINT(TextInputCtrl::paintEvent)
EVT_SIZE(TextInputCtrl::OnSize)
END_EVENT_TABLE()

TextInputCtrl::TextInputCtrl(wxWindow* parent, int id, wxString text, const wxPoint& pos /*= wxDefaultPosition*/, const wxSize& size /*= wxDefaultSize*/, long style /*= 0*/)
{
    radius = 4;
    border_width = 1;
    border_color = StateColor(std::make_pair(0xDBDBDB, (int)StateColor::Disabled), std::make_pair(0x15BF59, (int)StateColor::Hovered),
        std::make_pair(0xDBDBDB, (int)StateColor::Normal));
    background_color = StateColor(std::make_pair(0xF0F0F1, (int)StateColor::Disabled), std::make_pair(*wxWHITE, (int)StateColor::Normal));
    text_color = (std::make_pair(0x909090, (int)StateColor::Disabled),
        std::make_pair(0x262E30, (int)StateColor::Normal));

    StaticBox::Create(parent, wxID_ANY, pos, size, style);

    text_ctrl = new TextCtrl(this, wxID_ANY, text, wxDefaultPosition , wxDefaultSize, style | wxBORDER_NONE);
    text_ctrl->SetBackgroundColour(background_color.colorForStates(state_handler.states()));
    text_ctrl->SetForegroundColour(text_color.colorForStates(state_handler.states()));

    wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    this->SetSizer(sizer);
    sizer->Add(text_ctrl, 1, wxEXPAND|wxTOP|wxLEFT|wxRIGHT|wxBOTTOM, 4);


    text_ctrl->Bind(wxEVT_KILL_FOCUS, [this](auto& e) {
        e.SetId(GetId());
        ProcessEventLocally(e);
        e.Skip();
        });
    text_ctrl->Bind(wxEVT_TEXT_ENTER, [this](auto& e) {
        e.SetId(GetId());
        ProcessEventLocally(e);
        });

    
}

void TextInputCtrl::paintEvent(wxPaintEvent& evt)
{
    wxPaintDC dc(this);
    StaticBox::render(dc);
}

void TextInputCtrl::Rescale()
{
}

void TextInputCtrl::OnSize(wxSizeEvent& event)
{
    event.Skip();
    Rescale();
}