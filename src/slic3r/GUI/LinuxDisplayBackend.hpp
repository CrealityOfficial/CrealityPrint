#pragma once

namespace Slic3r {
namespace GUI {

#if defined(__WXGTK__)

enum class LinuxDisplayBackend { X11, Wayland, Unknown };

LinuxDisplayBackend get_linux_display_backend();
bool is_running_on_wayland();
bool is_running_on_x11();

#endif // defined(__WXGTK__)

} // namespace GUI
} // namespace Slic3r
