// Minimal panel for Supports settings (simple UI)
#pragma once

namespace Slic3r {
namespace GUI {

class GLCanvas3D;

namespace SupportSimple {
    // Render the support type tiles (5 options with images) inside current ImGui window.
    // Writes back to enable_support / support_type when user selects.
    // x,y,bottom_limit are kept for future positioning compatibility (currently unused).
    void render_simple_input_window();
}

} // namespace GUI
} // namespace Slic3r

