#ifndef slic3r_GUI_OfficialFilamentColorDialog_hpp_
#define slic3r_GUI_OfficialFilamentColorDialog_hpp_

#include "GUI_Utils.hpp"

#include <wx/bitmap.h>
#include <wx/colour.h>
#include <wx/timer.h>

#include <string>
#include <vector>
#include <nlohmann/json_fwd.hpp>

class wxBitmapButton;
class wxStaticBitmap;
class wxStaticText;

namespace Slic3r { namespace GUI {

namespace FilamentColorAppearance {
struct Appearance {
    std::vector<wxColour> colors;
    bool gradient = false;
    bool special() const { return colors.size() > 1 || (!colors.empty() && colors.front().Alpha() < 255); }
};

wxColour parse(const std::string& value);
bool equal(const wxColour& a, const wxColour& b);
bool is_gradient(const nlohmann::json& item);
Appearance resolve(size_t slot, const wxColour& base);
wxColour pixel(const Appearance& appearance, int x, int y, int width, int tile);
wxBitmap bitmap(const Appearance& appearance, int width, int height);
wxColour foreground(const Appearance& appearance);
}

class OfficialFilamentColorDialog final : public DPIDialog
{
public:
    OfficialFilamentColorDialog(wxWindow* parent, const std::string& filament_id,
                                const wxColour& current_color, const wxString& filament_name,
                                const std::vector<wxColour>& current_colors = {},
                                bool current_is_gradient = false);

    bool IsDataLoaded() const { return !m_entries.empty(); }
    wxColour GetSelectedColour() const { return m_selected_color; }
    std::vector<wxColour> GetSelectedColours() const;
    bool IsSelectedGradient() const;

protected:
    void on_dpi_changed(const wxRect& suggested_rect) override;

private:
    struct ColorEntry {
        wxString name;
        std::vector<wxColour> colors;
        bool gradient {false};
    };

    bool load_entries(const std::string& filament_id);
    void create_ui(const wxString& filament_name);
    void select_entry(size_t index);
    void select_custom_color(const wxColour& color);
    void update_preview(const ColorEntry* entry, const wxColour& custom_color = wxNullColour);
    void refresh_buttons();
    wxBitmap make_swatch(const ColorEntry& entry, const wxSize& size, bool selected = false) const;
    wxBitmap make_swatch(const wxColour& color, const wxSize& size, bool selected = false) const;

    std::vector<ColorEntry> m_entries;
    std::vector<wxBitmapButton*> m_buttons;
    wxColour m_selected_color;
    int m_selected_index {-1};
    bool m_is_dark {false};
    bool m_child_dialog_open {false};
    bool m_outside_click_armed {false};
    wxTimer m_outside_click_timer;
    wxStaticBitmap* m_preview {nullptr};
    wxStaticText* m_color_label {nullptr};
};

}} // namespace Slic3r::GUI

#endif
