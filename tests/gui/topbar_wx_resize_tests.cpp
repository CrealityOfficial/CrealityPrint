#include "../../src/slic3r/GUI/TopbarLayout.hpp"
#include <wx/wx.h>
#include <wx/aui/auibar.h>
#include <cstdlib>
#include <iostream>

using namespace Slic3r::GUI;

class ResizeTestApp : public wxApp {
public:
    bool OnInit() override { return true; }
};
wxIMPLEMENT_APP_NO_MAIN(ResizeTestApp);

static void require(bool condition, const char* message, int width)
{
    if (!condition) {
        std::cerr << message << " at width " << width << '\n';
        std::exit(1);
    }
}

// Exercise the real wxAuiToolBar sizer and hit-testing rather than assumed widths.
class ResizeTestBar : public wxAuiToolBar {
public:
    ResizeTestBar(wxWindow* parent, int scale)
        : wxAuiToolBar(parent, wxID_ANY, wxDefaultPosition, wxSize(400, 40 * scale),
                      wxAUI_TB_TEXT | wxAUI_TB_HORZ_TEXT | wxAUI_TB_NO_AUTORESIZE),
          m_scale(scale)
    {
        const wxBitmap icon(17 * scale, 17 * scale);
        for (int i = 0; i < 8; ++i) {
            AddTool(100 + i, "", icon);
            AddSpacer(10 * scale);
        }
        m_leading_spacer = AddStretchSpacer(1);
        m_tabs = new wxControl(this, wxID_ANY);
        m_tabs_item = AddControl(m_tabs);
        m_spacer = AddStretchSpacer(1);
        m_mode = new wxControl(this, wxID_ANY);
        m_mode->SetMinSize(wxSize(70 * scale, 24 * scale));
        m_mode_item = AddControl(m_mode);
        m_mode_item->SetMinSize(m_mode->GetMinSize());
        m_feedback = AddTool(200, "", icon);
        m_more = AddTool(201, "", icon);
        for (int id = 300; id < 303; ++id)
            AddTool(id, "", icon);

        Apply(TopbarLayout::Normal);
        widths.normal = ContentWidth();
        Apply(TopbarLayout::Compact);
        widths.compact = ContentWidth();
        Apply(TopbarLayout::Overflow);
        widths.overflow = ContentWidth();
        SetMinSize(wxSize(widths.overflow, 40 * scale));
    }

    int ContentWidth() const { return m_sizer->GetMinSize().x; }

    void CheckWidth(int width)
    {
        Apply(widths.layout_for(width));
        SetMinSize(wxSize(widths.overflow, 40 * m_scale));
        SetSize(width, 40 * m_scale);
        for (int id = 300; id < 303; ++id)
            require(GetToolFits(id), "Window control was clipped", width);
        require(m_tabs->GetRect().GetRight() < GetClientSize().x, "Tabs were clipped", width);
        const int leading_width = m_leading_spacer->GetSizerItem()->GetSize().x;
        const int trailing_width = m_spacer->GetSizerItem()->GetSize().x;
        require(std::abs(leading_width - trailing_width) <= 1, "Tabs were not centered", width);
    }

    TopbarWidths widths;

private:
    void Apply(TopbarLayout layout)
    {
        const bool collapsed = layout == TopbarLayout::Overflow;
        m_tabs->SetMinSize(wxSize((layout == TopbarLayout::Normal ? 525 : 430) * m_scale, 30 * m_scale));
        m_tabs_item->SetMinSize(m_tabs->GetMinSize());
        m_mode_item->SetKind(collapsed ? m_spacer->GetKind() : m_tabs_item->GetKind());
        m_mode->Show(!collapsed);
        m_feedback->SetKind(collapsed ? m_spacer->GetKind() : wxITEM_NORMAL);
        m_more->SetKind(collapsed ? wxITEM_NORMAL : m_spacer->GetKind());
        Realize();
    }

    int m_scale;
    wxControl *m_tabs, *m_mode;
    wxAuiToolBarItem *m_tabs_item, *m_leading_spacer, *m_spacer, *m_mode_item, *m_feedback, *m_more;
};

int main(int argc, char** argv)
{
    if (!wxEntryStart(argc, argv))
        return 1;
    wxTheApp->CallOnInit();
    for (int scale : {1, 2}) {
        auto* frame = new wxFrame(nullptr, wxID_ANY, "hidden toolbar resize test");
        auto* bar = new ResizeTestBar(frame, scale);
        const int smallest = bar->widths.overflow;
        const int largest = bar->widths.normal + 800 * scale;
        require(smallest < bar->widths.compact && bar->widths.compact < bar->widths.normal,
                "Real toolbar layouts must have distinct content widths", smallest);
        for (int width = largest; width >= smallest; --width)
            bar->CheckWidth(width);
        for (int width = smallest; width <= largest; ++width)
            bar->CheckWidth(width);
        std::cout << "scale=" << scale << " normal=" << bar->widths.normal
                  << " compact=" << bar->widths.compact << " overflow=" << smallest
                  << ": tabs centered and all window buttons fit throughout shrink/grow\n";
        delete frame;
    }
    wxTheApp->OnExit();
    wxEntryCleanup();
}